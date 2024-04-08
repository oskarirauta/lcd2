#pragma once

#include <string>
#include <vector>
#include <map>

#include "config.hpp"
#include "tsl/ordered_map.h"
#include "properties.hpp"
#include "rgb.hpp"
#include "orientation.hpp"
#include "driver_classes.hpp"
#include "widget_classes.hpp"
#include "plugin_classes.hpp"
#include "timer.hpp"
#include "action.hpp"
#include "layout.hpp"
#include "scheduler.hpp"

class DISPLAY : public PROPERTIES {

	friend class drv::DRIVER;
	friend class drv::DPF;

	friend class LAYOUT;
	friend class SCHEDULER;

	protected:

		ORIENTATION _orientation;
		int _backlight;
		int _page = 0;
		int _width, _height;

		std::vector<RGBA> new_layer();

	private:
		bool _clean_up = true;

		void init_variables(CONFIG::MAP* cfg);
		void init_display(CONFIG::MAP* cfg);
		void init_timers(CONFIG::MAP* cfg);
		void init_widgets(CONFIG::MAP* cfg);
		void init_layout(CONFIG::MAP* cfg);
		void init_canvas();
		void init_driver(const std::string& driver, const std::string& device);
		void init_scheduler(CONFIG::MAP* cfg);

		void clean_up();

	public:

		std::map<int, std::map<int, std::vector<RGBA>>> canvas;
		std::vector<LAYOUT::WIDGET_LINK> updated_widgets;

		drv::DRIVER *driver = nullptr;
		widget *widgets = nullptr;
		plugin *plugins = nullptr;
		action *actions = nullptr;
		common::lowercase_map<TIMER> timers;
		LAYOUT *layout = nullptr;
		SCHEDULER *scheduler = nullptr;

		int width();
		int height();
		int pwidth();
		int pheight();
		ORIENTATION orientation();
		void orientation(ORIENTATION orientation);
		int backlight();
		void backlight(int value);
		void clear();
		int page_number();
		bool page_number(int page_no);
		void refresh();
		void add_pixel(int x, int y, int page, int layer, RGBA color);

		bool setpage(int page_no);
		bool goodbye();
		bool threading();
		void run();

		DISPLAY& operator *() { return *this; };
		DISPLAY* operator ->() { return this; }

		DISPLAY(CONFIG* cfg);
		~DISPLAY();

		friend std::ostream& operator <<(std::ostream& os, DISPLAY& d);
		friend std::ostream& operator <<(std::ostream& os, DISPLAY *d);
};

std::ostream& operator <<(std::ostream& os, DISPLAY& d);
std::ostream& operator <<(std::ostream& os, DISPLAY *d);

extern DISPLAY *display;
