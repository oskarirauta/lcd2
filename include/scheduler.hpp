#pragma once

#include <atomic>
#include <thread>
#include <mutex>
#include <stop_token>

#include "layout.hpp"

class DISPLAY;
class SCHEDULER {

	friend class DISPLAY;

	private:

		bool run_once();

		static void update_plugins(std::stop_token token, SCHEDULER& sched);
		static void update_timers(std::stop_token token, SCHEDULER& sched);
		static void update_widgets(std::stop_token token, SCHEDULER& sched);
		static void update_layout(std::stop_token token, SCHEDULER& sched);
		static void update_display(std::stop_token token, SCHEDULER& sched);

		void run_unthreaded(bool once = false);
		void run_threaded();

	protected:

		int current_page;
		bool _exit_loop = false;
		DISPLAY *display = nullptr;
		bool _run_once = true;
		bool _is_threaded = false;

	public:

		bool threading();
		void exit_loop(bool value);
		bool exit_loop();
		void run();

		SCHEDULER(DISPLAY *display, bool threaded = true);
		~SCHEDULER();
};
