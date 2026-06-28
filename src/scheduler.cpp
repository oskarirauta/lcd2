#include <chrono>
#include <stdexcept>

#include "logger.hpp"
#include "throws.hpp"
#include "driver.hpp"
#include "plugin_classes.hpp"
#include "widget_classes.hpp"
#include "display.hpp"
#include "timer.hpp"
#include "layout.hpp"
#include "scheduler.hpp"

// Data thread update interval: frequent enough for any plugin's own interval check.
static constexpr auto DATA_INTERVAL   = std::chrono::milliseconds(100);

// Render thread target frame time (~30 fps).
static constexpr auto RENDER_INTERVAL = std::chrono::milliseconds(33);

// How often the main wait loop polls for the stop flag.
static constexpr auto MAIN_POLL       = std::chrono::milliseconds(100);

SCHEDULER::SCHEDULER(DISPLAY* display, bool threaded)
    : _display(display), _is_threaded(threaded) {}

SCHEDULER::~SCHEDULER() {
    _display = nullptr;
}

bool SCHEDULER::threading() const {
    return _is_threaded;
}

void SCHEDULER::exit_loop(bool value) {
    _stop.store(value, std::memory_order_relaxed);
}

bool SCHEDULER::exit_loop() const {
    return _stop.load(std::memory_order_relaxed);
}

// ── Initialisation (called once in main thread before threads start) ─────────

bool SCHEDULER::run_once() {

    _display->clean_up();

    if (_display->canvas.empty())
        _display->init_canvas();

    _display->clear();
    return true;
}

// ── Pipeline helpers ──────────────────────────────────────────────────────────

void SCHEDULER::update_plugins() {

    for (auto it = _display->plugins->begin(); it != _display->plugins->end(); ++it) {
        auto tp0 = std::chrono::steady_clock::now();
        it->second->update();
        auto tp_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - tp0).count();
        if (tp_ms > 50)
            logger::verbose["scheduler"] << "plugin '" << it->first << "' update took "
                << tp_ms << "ms" << std::endl;
    }
}

void SCHEDULER::update_timers() {

    int page = _current_page.load(std::memory_order_relaxed);

    for (auto& [key, _] : _display->timers) {
        TIMER& t = _display->timers[key];
        if (t.is_global(_display->layout) || t.on_page(_display->layout, page))
            t.update();
    }
}

bool SCHEDULER::update_widgets() {

    int page = _current_page.load(std::memory_order_relaxed);
    bool any_updated = false;

    if (!_display->layout->pages.contains(page))
        return false;

    for (auto& [k, layer] : _display->layout->pages[page].layers) {
        if (_stop.load(std::memory_order_relaxed)) break;
        for (auto& wlink : layer.widgets) {
            if (_stop.load(std::memory_order_relaxed)) break;
            auto tw0 = std::chrono::steady_clock::now();
            bool updated = wlink.update();
            if (updated) any_updated = true;
            auto tw_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - tw0).count();
            if (tw_ms > 50)
                logger::verbose["scheduler"] << "widget '" << wlink.name << "' update took "
                    << tw_ms << "ms" << std::endl;
        }
    }

    return any_updated;
}

// ── Data thread: plugins + timers ─────────────────────────────────────────────

void SCHEDULER::data_loop(std::stop_token token) {

    while (!token.stop_requested() && !_stop.load(std::memory_order_relaxed)) {

        auto next = std::chrono::steady_clock::now() + DATA_INTERVAL;

        {
            auto t0 = std::chrono::steady_clock::now();
            std::lock_guard<std::mutex> lock(_data_mutex);
            update_plugins();
            update_timers();
            auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - t0).count();
            if (elapsed_ms > 50)
                logger::verbose["scheduler"] << "data: plugins+timers=" << elapsed_ms << "ms" << std::endl;
        }

        std::this_thread::sleep_until(next);
    }
}

// ── Render thread: widgets → layout → display ─────────────────────────────────

void SCHEDULER::render_loop(std::stop_token token) {

    long _frame = 0;

    while (!token.stop_requested() && !_stop.load(std::memory_order_relaxed)) {

        auto frame_start = std::chrono::steady_clock::now();

        if (++_frame % 300 == 0)
            logger::debug["scheduler"] << "render alive, frame=" << _frame << std::endl;

        // Snapshot current page; exceptions mean display is shutting down
        try {
            _current_page.store(_display->page_number(), std::memory_order_relaxed);
        } catch (...) {
            std::this_thread::sleep_for(RENDER_INTERVAL);
            continue;
        }

        // Widget update evaluates expressions → reads CONFIG::variables → needs lock
        bool any_updated = false;
        {
            auto t0 = std::chrono::steady_clock::now();
            std::lock_guard<std::mutex> lock(_data_mutex);
            auto t1 = std::chrono::steady_clock::now();
            any_updated = update_widgets();
            auto t2 = std::chrono::steady_clock::now();
            auto lock_wait = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();
            auto widget_ms = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count();
            if (lock_wait > 50 || widget_ms > 50)
                logger::verbose["scheduler"] << "render: lock_wait=" << lock_wait
                    << "ms update_widgets=" << widget_ms << "ms" << std::endl;
        }

        // layout->render() + refresh() must be serialised against the data thread:
        // a timer action there can call setpage(), which runs the SAME render/blit
        // pipeline (update_widgets + layout->render + refresh). Without the lock the
        // two threads concurrently mutate layout->pages widget lists, display->canvas
        // and widget bitmaps -> iterator invalidation / use-after-free. Share
        // _data_mutex (setpage runs under it on the data thread, so they exclude).
        if (any_updated && !_stop.load(std::memory_order_relaxed)) {
            std::lock_guard<std::mutex> lock(_data_mutex);
            auto t3 = std::chrono::steady_clock::now();
            _display->layout->render();
            auto t4 = std::chrono::steady_clock::now();
            _display->refresh();
            auto t5 = std::chrono::steady_clock::now();
            auto render_ms  = std::chrono::duration_cast<std::chrono::milliseconds>(t4 - t3).count();
            auto refresh_ms = std::chrono::duration_cast<std::chrono::milliseconds>(t5 - t4).count();
            if (render_ms > 50 || refresh_ms > 50)
                logger::verbose["scheduler"] << "render: layout=" << render_ms
                    << "ms refresh=" << refresh_ms << "ms" << std::endl;
        }

        auto elapsed  = std::chrono::steady_clock::now() - frame_start;
        auto leftover = RENDER_INTERVAL - elapsed;
        if (leftover > std::chrono::milliseconds(0))
            std::this_thread::sleep_for(leftover);
    }
}

// ── Threaded main loop ─────────────────────────────────────────────────────────

void SCHEDULER::run_threaded() {

    _data_thread   = std::jthread([this](std::stop_token t){ data_loop(t); });
    _render_thread = std::jthread([this](std::stop_token t){ render_loop(t); });

    logger::verbose["scheduler"] << "threads started" << std::endl;

    while (!_stop.load(std::memory_order_relaxed))
        std::this_thread::sleep_for(MAIN_POLL);

    _data_thread.request_stop();
    _render_thread.request_stop();
    _data_thread.join();
    _render_thread.join();
}

// ── Unthreaded main loop ───────────────────────────────────────────────────────

void SCHEDULER::run_unthreaded() {

    int cycle = 0;

    while (!_stop.load(std::memory_order_relaxed)) {

        auto start = std::chrono::steady_clock::now();

        try {
            _current_page.store(_display->page_number(), std::memory_order_relaxed);
        } catch (...) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }

        update_plugins();
        update_timers();

        bool any_updated = update_widgets();

        if (any_updated && !_stop.load(std::memory_order_relaxed)) {
            auto tr0 = std::chrono::steady_clock::now();
            _display->layout->render();
            auto tr1 = std::chrono::steady_clock::now();
            _display->refresh();
            auto tr2 = std::chrono::steady_clock::now();
            auto layout_ms  = std::chrono::duration_cast<std::chrono::milliseconds>(tr1 - tr0).count();
            auto refresh_ms = std::chrono::duration_cast<std::chrono::milliseconds>(tr2 - tr1).count();
            if (layout_ms > 50 || refresh_ms > 50)
                logger::verbose["scheduler"] << "unthreaded: layout=" << layout_ms
                    << "ms refresh=" << refresh_ms << "ms" << std::endl;
        }

        auto elapsed = std::chrono::steady_clock::now() - start;

        logger::verbose["scheduler"] << "cycle #" << cycle++
            << " " << std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count()
            << "ms (updated=" << any_updated << ")" << std::endl;

        // Target ~600 ms cycle; sleep the remainder (minimum 50 ms)
        auto leftover = std::chrono::milliseconds(600) - elapsed;
        if (leftover > std::chrono::milliseconds(50))
            std::this_thread::sleep_for(leftover);
        else
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
}

// ── Entry point ───────────────────────────────────────────────────────────────

void SCHEDULER::run() {

    if (_stop.load(std::memory_order_relaxed)) {
        logger::verbose["scheduler"] << "stop requested before loop started" << std::endl;
        _display = nullptr;
        return;
    }

    // Wait until display and layout are ready (populated by DISPLAY constructor)
    while ((_display == nullptr || _display->layout == nullptr)
           && !_stop.load(std::memory_order_relaxed)) {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    if (_stop.load(std::memory_order_relaxed)) return;

    run_once();

    if (_stop.load(std::memory_order_relaxed)) return;

    logger::verbose["scheduler"] << "starting ("
        << (_is_threaded ? "threaded" : "unthreaded") << ")" << std::endl;

    // Initial snapshot + render. page_number() throws (via throws<<) when the
    // current page is absent from the canvas (e.g. a misconfigured start page
    // whose layers reference only missing widgets); guard it so the exception
    // cannot escape run() and terminate the process. The main loops below have
    // their own guards around page_number() for the same reason.
    try {
        _current_page.store(_display->page_number(), std::memory_order_relaxed);

        // Single initial render so the display shows something immediately
        update_plugins();
        update_timers();
        update_widgets();
        _display->layout->render();
        _display->refresh();
    } catch (const std::exception& e) {
        logger::warning["scheduler"] << "initial render skipped: " << e.what() << std::endl;
    }

    if (_stop.load(std::memory_order_relaxed)) return;

    try {
        if (_is_threaded)
            run_threaded();
        else
            run_unthreaded();
    } catch (const std::exception& e) {
        logger::error["scheduler"] << "loop exited abnormally: " << e.what() << std::endl;
    }

    logger::verbose["scheduler"] << "stopped" << std::endl;
}
