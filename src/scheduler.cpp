#include <thread>
#include <chrono>
#include <atomic>
#include <poll.h>

#include "logger.hpp"
#include "throws.hpp"
#include "driver.hpp"
#include "plugin_classes.hpp"
#include "widget_classes.hpp"
#include "display.hpp"
#include "timer.hpp"
#include "layout.hpp"
#include "scheduler.hpp"

static std::atomic_bool lock_plugins = ATOMIC_VAR_INIT(false);
static std::atomic_bool needs_plugins_update = ATOMIC_VAR_INIT(false);

static std::atomic_bool lock_global_timers = ATOMIC_VAR_INIT(false);
static std::atomic_bool needs_global_timers_update = ATOMIC_VAR_INIT(false);

static std::atomic_bool lock_timers = ATOMIC_VAR_INIT(false);
static std::atomic_bool needs_timers_update = ATOMIC_VAR_INIT(false);

static std::atomic_bool lock_widgets = ATOMIC_VAR_INIT(false);
static std::atomic_bool needs_widgets_update = ATOMIC_VAR_INIT(false);
static std::atomic_bool widgets_updated = ATOMIC_VAR_INIT(false);

static std::atomic_bool lock_layout = ATOMIC_VAR_INIT(false);
static std::atomic_bool needs_layout_update = ATOMIC_VAR_INIT(false);
static std::atomic_bool layout_updated = ATOMIC_VAR_INIT(false);

static std::atomic_bool lock_display = ATOMIC_VAR_INIT(false);
static std::atomic_bool needs_display_update = ATOMIC_VAR_INIT(false);

static std::atomic_bool is_threaded = ATOMIC_VAR_INIT(true);
static std::atomic_bool needs_exit = ATOMIC_VAR_INIT(false);

SCHEDULER::SCHEDULER(DISPLAY *display, bool threaded) {

	this -> display = display;
	is_threaded.store(threaded, std::memory_order_relaxed);
}

SCHEDULER::~SCHEDULER() {

	this -> display = nullptr;
}

void SCHEDULER::exit_loop(bool value) {

	needs_exit.store(value, std::memory_order_relaxed);
}

bool SCHEDULER::exit_loop() {

	return needs_exit.load(std::memory_order_relaxed);
}

static int sleep_ms(long int ms) {

	if ( ms < 1 )
		return 1;
	else if ( ms > 2500 ) {
		logger::debug["scheduler"] << "attempt to sleep more than 2500ms, " << ms << " to be precise.. Ignoring sleep request" << std::endl;
		return 1;
	}

	return ::poll(NULL, 0, ms);
}

void SCHEDULER::update_plugins(std::stop_token token, SCHEDULER& sched) {

	while ( !token.stop_requested() && !needs_exit.load(std::memory_order_relaxed)) {

		if ( is_threaded.load(std::memory_order_relaxed) && !needs_plugins_update.load(std::memory_order_relaxed) && !token.stop_requested()) {

			sleep_ms(250);
			//std::this_thread::sleep_for(std::chrono::milliseconds(250));
			continue;
		} else if ( is_threaded.load(std::memory_order_relaxed) && token.stop_requested()) break;

		lock_plugins.store(true, std::memory_order_relaxed);

		for ( auto it = sched.display -> plugins -> begin(); it != sched.display -> plugins -> end() && !token.stop_requested(); it++ )
			it -> second.get() -> update();

		needs_plugins_update.store(false, std::memory_order_relaxed);
		lock_plugins.store(false, std::memory_order_relaxed);

		if ( !is_threaded.load(std::memory_order_relaxed) )
			break;
		else if ( is_threaded.load(std::memory_order_relaxed) && !token.stop_requested())
			sleep_ms(140);
			//std::this_thread::sleep_for(std::chrono::milliseconds(140));
	}
}

void SCHEDULER::update_timers(std::stop_token token, SCHEDULER& sched) {

	while ( !token.stop_requested() && !needs_exit.load(std::memory_order_relaxed)) {

		if ( is_threaded.load(std::memory_order_relaxed) && !needs_timers_update.load(std::memory_order_relaxed) && !token.stop_requested()) {

			sleep_ms(120);
			//std::this_thread::sleep_for(std::chrono::milliseconds(120));
			continue;
		} else if ( is_threaded.load(std::memory_order_relaxed) && token.stop_requested()) break;

		lock_timers.store(true, std::memory_order_relaxed);

		while ( is_threaded.load(std::memory_order_relaxed) && !token.stop_requested() && lock_widgets.load(std::memory_order_relaxed));

		for ( const auto& [key, _] : sched.display -> timers ) {

			if ( sched.display -> timers[key].is_global(sched.display -> layout))
				sched.display -> timers[key].update();
			else if ( sched.display -> timers[key].on_page(sched.display -> layout, sched.current_page))
				sched.display -> timers[key].update();

			if ( !is_threaded.load(std::memory_order_relaxed) )
				continue;
			else if ( token.stop_requested())
				break;
		}

		needs_timers_update.store(false, std::memory_order_relaxed);
		lock_timers.store(false, std::memory_order_relaxed);

		if ( !is_threaded.load(std::memory_order_relaxed) )
			break;
		else if ( is_threaded.load(std::memory_order_relaxed) && !token.stop_requested())
			sleep_ms(80);
			//std::this_thread::sleep_for(std::chrono::milliseconds(80));
	}
}

void SCHEDULER::update_widgets(std::stop_token token, SCHEDULER& sched) {

	int page_no;

	while ( !token.stop_requested() && !needs_exit.load(std::memory_order_relaxed)) {

		if ( is_threaded.load(std::memory_order_relaxed) && !token.stop_requested()) {

			if ( !needs_widgets_update.load(std::memory_order_relaxed) ||
				widgets_updated.load(std::memory_order_relaxed)) {

				sleep_ms(30);
				//std::this_thread::sleep_for(std::chrono::milliseconds(30));
				continue;
			}
		} else if ( is_threaded.load(std::memory_order_relaxed) && token.stop_requested()) break;

		lock_widgets.store(true, std::memory_order_relaxed);

		page_no = sched.current_page;

		while ( is_threaded.load(std::memory_order_relaxed) && !token.stop_requested() && widgets_updated.load(std::memory_order_relaxed) &&
			( lock_layout.load(std::memory_order_relaxed) || lock_display.load(std::memory_order_relaxed) ||
				lock_timers.load(std::memory_order_relaxed)));

		for ( auto& [k, layer] : sched.display -> layout -> pages[page_no].layers ) {

			if ( is_threaded.load(std::memory_order_relaxed) && ( token.stop_requested() || needs_exit.load(std::memory_order_relaxed)))
				break;

			for ( auto& widget : layer.widgets ) {

				if ( is_threaded.load(std::memory_order_relaxed) && ( token.stop_requested() || needs_exit.load(std::memory_order_relaxed)))
					break;
				else if ( widget.update())
					widgets_updated.store(true, std::memory_order_relaxed);
			}
		}

		needs_widgets_update.store(false, std::memory_order_relaxed);
		lock_widgets.store(false, std::memory_order_relaxed);

		if ( !is_threaded.load(std::memory_order_relaxed) )
			break;
		else if ( is_threaded.load(std::memory_order_relaxed) && !token.stop_requested())
			sleep_ms(10);
			//std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}
}

void SCHEDULER::update_layout(std::stop_token token, SCHEDULER& sched) {

	while ( !token.stop_requested() && !needs_exit.load(std::memory_order_relaxed)) {

		if ( is_threaded.load(std::memory_order_relaxed) && !token.stop_requested()) {

			if ( !needs_layout_update.load(std::memory_order_relaxed) ||
				!widgets_updated.load(std::memory_order_relaxed) ||
				layout_updated.load(std::memory_order_relaxed)) {

				sleep_ms(30);
				//std::this_thread::sleep_for(std::chrono::milliseconds(30));
				continue;
			}
		} else if ( is_threaded.load(std::memory_order_relaxed) && token.stop_requested()) break;

		lock_layout.store(true, std::memory_order_relaxed);

		while ( is_threaded.load(std::memory_order_relaxed) && !token.stop_requested() && layout_updated.load(std::memory_order_relaxed) &&
			( lock_widgets.load(std::memory_order_relaxed) || lock_display.load(std::memory_order_relaxed)));

		if ( is_threaded.load(std::memory_order_relaxed) && token.stop_requested())
			break;

		sched.display -> layout -> render();
		layout_updated.store(true, std::memory_order_relaxed);

		needs_layout_update.store(false, std::memory_order_relaxed);
		lock_layout.store(false, std::memory_order_relaxed);

		if ( !is_threaded.load(std::memory_order_relaxed) )
			break;
		else if ( is_threaded.load(std::memory_order_relaxed) && !token.stop_requested())
			sleep_ms(10);
			//std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}
}

void SCHEDULER::update_display(std::stop_token token, SCHEDULER& sched) {

	while ( !token.stop_requested() && !needs_exit.load(std::memory_order_relaxed)) {

		if ( is_threaded.load(std::memory_order_relaxed) && !token.stop_requested()) {

			if ( !needs_display_update.load(std::memory_order_relaxed) ||
				!widgets_updated.load(std::memory_order_relaxed) ||
				!layout_updated.load(std::memory_order_relaxed)) {

				sleep_ms(30);
				//std::this_thread::sleep_for(std::chrono::milliseconds(30));
				continue;
			}

		} else if ( is_threaded.load(std::memory_order_relaxed) && token.stop_requested()) break;

		lock_display.store(true, std::memory_order_relaxed);

		while ( is_threaded.load(std::memory_order_relaxed) && !token.stop_requested() &&
			( lock_widgets.load(std::memory_order_relaxed) || lock_layout.load(std::memory_order_relaxed)));

		if ( is_threaded.load(std::memory_order_relaxed) && token.stop_requested())
			break;

		sched.display -> refresh();
		layout_updated.store(false, std::memory_order_relaxed);
		widgets_updated.store(false, std::memory_order_relaxed);

		needs_display_update.store(false, std::memory_order_relaxed);
		lock_display.store(false, std::memory_order_relaxed);

		if ( !is_threaded.load(std::memory_order_relaxed) )
			break;
		else if ( is_threaded.load(std::memory_order_relaxed) && !token.stop_requested())
			sleep_ms(10);
			//std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}
}

bool SCHEDULER::run_once() {

	if ( !this -> _run_once )
		return false;

	this -> _run_once = false;

	this -> display -> clean_up();

	if ( this -> display -> canvas.empty())
		this -> display -> init_canvas();

	this -> display -> clear();

	return true;
}

void SCHEDULER::run_unthreaded(bool once) {

	int cycle = 0;
	bool was_threaded = is_threaded.load(std::memory_order_relaxed);
	std::stop_token token;
	[[ maybe_unused ]] bool needs_delay;

	if ( once )
		is_threaded.store(false, std::memory_order_relaxed);

	sleep_ms(100);
	//std::this_thread::sleep_for(std::chrono::milliseconds(100));

	bool widgets_updated = once ? true : false;

	while ( !needs_exit.load(std::memory_order_relaxed)) {

		needs_delay = true;

		auto start = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());
		this -> current_page = this -> display -> page_number();

		// update plugins
		for ( auto it = this -> display -> plugins -> begin(); it != this -> display -> plugins -> end() && !needs_exit.load(std::memory_order_relaxed); it++ )
			it -> second.get() -> update();
		auto plugins_end = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());

		// update timers
		for ( const auto& [key, _] : this -> display -> timers ) {

			if ( needs_exit.load(std::memory_order_relaxed))
				break;
			else if ( this -> display -> timers[key].is_global(this -> display -> layout) ||
				this -> display -> timers[key].on_page(this -> display -> layout, this -> current_page))
				this -> display -> timers[key].update();
		}
		auto timers_end = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());

		// update widgets
		widgets_updated = false;
		for ( auto& [k, layer] : this -> display -> layout -> pages[this -> current_page].layers ) {

			if ( needs_exit.load(std::memory_order_relaxed))
				break;

                        for ( auto& widget : layer.widgets ) {

                                if ( needs_exit.load(std::memory_order_relaxed)) break;
				else {
					auto *w = widget.get_ptr();
					if ( w -> update()) widgets_updated = true;
					else if ( w -> reloads()) sleep_ms(8);
					//if ( widget.update()) widgets_updated = true;
				}
			}
		}
		auto widgets_end = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());

		// update layout
		if ( widgets_updated && !needs_exit.load(std::memory_order_relaxed))
			this -> display -> layout -> render();
		auto layout_end = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());

		// update display
		if ( widgets_updated && !needs_exit.load(std::memory_order_relaxed)) {

			this -> display -> refresh();
			widgets_updated = false;
		}
		auto display_end = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());

		if ( once ) {
			is_threaded.store(was_threaded, std::memory_order_relaxed);
			break;
		}

		//if ( needs_delay) // add small delay
		//	sleep_ms(10);
		//	std::this_thread::sleep_for(std::chrono::milliseconds(10));

		if ( needs_exit.load(std::memory_order_relaxed))
			break;

		if (( display_end - start ).count() < 550 )
			sleep_ms( 600 - (display_end - start).count());
		else sleep_ms(250);
		//if ( needs_delay )
		//	sleep_ms(10);

		//sleep_ms(500);

		auto sleep_end = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());

		if ( !logger::silence ) /*
			std::cout << "\ncycle #" << cycle++ << ": " <<
				(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()) - start).count() <<
				"ms" << std::endl;
			*/

			std::cout << "\ncycle #" << cycle++ << ": " << ( sleep_end - start ).count() << "ms " <<
						"plugins: " << ( plugins_end - start ).count() << "ms " <<
						"timers: " << ( timers_end - plugins_end ).count() << "ms " <<
						"widgets: " << ( widgets_end - timers_end ).count() << "ms " <<
						"layout: " << ( layout_end - widgets_end ).count() << "ms " <<
						"display: " << ( display_end - widgets_end ).count() << "ms " <<
						"all: " << ( display_end - start ).count() << "ms " <<
						"sleep: " << ( sleep_end - display_end ).count() << "ms" << std::endl;

	}

}

void SCHEDULER::run_threaded() {

	int cycle = 0;

        std::jthread thread_plugins(SCHEDULER::update_plugins, std::ref(*this));
        std::jthread thread_timers(SCHEDULER::update_timers, std::ref(*this));
        std::jthread thread_widgets(SCHEDULER::update_widgets, std::ref(*this));
        std::jthread thread_layout(SCHEDULER::update_layout, std::ref(*this));
        std::jthread thread_display(SCHEDULER::update_display, std::ref(*this));

	// Small delay to let things settle to avoid segfaults from concurrent actions
	sleep_ms(400);
	//std::this_thread::sleep_for(std::chrono::milliseconds(400));

	while ( !needs_exit.load(std::memory_order_relaxed)) {

		auto start = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());

		this -> current_page = this -> display -> page_number();

		// update plugins
		if ( !lock_plugins.load(std::memory_order_relaxed) && !needs_plugins_update.load(std::memory_order_relaxed) &&
			!needs_exit.load(std::memory_order_relaxed))
			needs_plugins_update.store(true, std::memory_order_relaxed);

		// update timers
		if ( !lock_timers.load(std::memory_order_relaxed) && !needs_timers_update.load(std::memory_order_relaxed) &&
			!needs_exit.load(std::memory_order_relaxed))
			needs_timers_update.store(true, std::memory_order_relaxed);

		// update widgets
		if ( !lock_widgets.load(std::memory_order_relaxed) && !needs_widgets_update.load(std::memory_order_relaxed) &&
			!widgets_updated.load(std::memory_order_relaxed) && !needs_exit.load(std::memory_order_relaxed))
			needs_widgets_update.store(true, std::memory_order_relaxed);

		// update layout
		if ( !lock_layout.load(std::memory_order_relaxed) && !needs_layout_update.load(std::memory_order_relaxed) &&
			widgets_updated.load(std::memory_order_relaxed) && !needs_exit.load(std::memory_order_relaxed))
			needs_layout_update.store(true, std::memory_order_relaxed);

		// update display
		if ( !lock_display.load(std::memory_order_relaxed) && !needs_display_update.load(std::memory_order_relaxed) &&
			widgets_updated.load(std::memory_order_relaxed) && layout_updated.load(std::memory_order_relaxed) &&
			!needs_exit.load(std::memory_order_relaxed))
			needs_display_update.store(true, std::memory_order_relaxed);

		// add small delay
		if ( needs_exit.load(std::memory_order_relaxed))
			break;

		auto now = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());
		if ( start + std::chrono::milliseconds(20) > now ) {
			long int c = 40 - ( now - start ).count();
			if ( c < 40 && c > 0 )	sleep_ms(c);
			//std::this_thread::sleep_for(std::chrono::milliseconds(40 - (now - start).count()));
		}

		if ( !logger::silence )
			std::cout << "\ncycle #" << cycle++ << ": " <<
				(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()) - start).count() <<
				"ms" << std::endl;
	}
}

bool SCHEDULER::threading() {

	return is_threaded.load(std::memory_order_relaxed);
}

void SCHEDULER::run() {

	if ( this -> exit_loop()) {

		logger::vverbose["scheduler"] << "exit in eraly stage called with signal" << std::endl;
		logger::verbose["scheduler"] << "exiting loop before it started" << std::endl;
		this -> display = nullptr;
		return;
	}

	this -> exit_loop(false);

	// TODO: throw if 1 minute has passed..
	while (( this -> display == nullptr || this -> display -> layout == nullptr ) && !this -> exit_loop());

	sleep_ms(250);
	//std::this_thread::sleep_for(std::chrono::milliseconds(250));

	if ( !this -> run_once())
		this -> display -> clear();

	if ( this -> exit_loop())
		return;
	else logger::verbose["scheduler"] << "starting scheduler" << std::endl;

	logger::verbose["scheduler"] << "threading " <<
		( is_threaded.load(std::memory_order_relaxed) ? "enabled" : "disabled" ) <<
		std::endl;

	this -> current_page = this -> display -> page_number();

	try {
		run_unthreaded(true);

	} catch ( const std::runtime_error& e ) {

		logger::verbose["scheduler"] << "exiting loop" << std::endl;
	}

	if ( this -> exit_loop()) {

		logger::verbose << "scheduler stopped" << std::endl;
		return;
	}

	logger::vverbose["scheduler"] << "loop begins" << std::endl;

	try {

		if ( is_threaded.load(std::memory_order_relaxed))
			run_threaded();
		else
			run_unthreaded();

		logger::verbose["scheduler"] << "exiting loop" << std::endl;

	} catch (const std::runtime_error& e ) {

		logger::error["scheduler"] << "loop exited abnormally, reason: " << e.what() << std::endl;
	}

	logger::verbose["scheduler"] << "ending loop" << std::endl;
}
