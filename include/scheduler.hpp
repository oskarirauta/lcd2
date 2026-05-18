#pragma once

#include <atomic>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <chrono>

#include "layout.hpp"

class DISPLAY;

class SCHEDULER {

    friend class DISPLAY;

    private:

        DISPLAY* _display = nullptr;
        bool _is_threaded = false;

        // Current page (atomic so data and render threads can read safely)
        std::atomic<int> _current_page{0};

        // Stop requested by signal or display shutdown
        std::atomic_bool _stop{false};

        // Protects CONFIG::variables and plugin/timer state across threads.
        // Data thread holds it exclusively during plugin+timer updates.
        // Render thread holds it during widget expression evaluation.
        std::mutex _data_mutex;

        // Worker threads (threaded mode only)
        std::jthread _data_thread;
        std::jthread _render_thread;

        bool run_once();

        void update_plugins();
        void update_timers();
        bool update_widgets();

        void data_loop(std::stop_token token);
        void render_loop(std::stop_token token);

        void run_threaded();
        void run_unthreaded();

    public:

        bool threading() const;

        void exit_loop(bool value);
        bool exit_loop() const;

        void run();

        SCHEDULER(DISPLAY* display, bool threaded = true);
        ~SCHEDULER();
};
