#include <iostream>
#include <stdexcept>
#include <algorithm>
#include <chrono>
#include <thread>

#include "common.hpp"
#include "logger.hpp"

#include "config.hpp"

#include "driver.hpp"
#include "drivers/dpf.hpp"

#include "rgb.hpp"
#include "plugin.hpp"
#include "expr/expression.hpp"
#include "throws.hpp"
#include "display.hpp"

DISPLAY *display = nullptr;

DISPLAY::DISPLAY(CONFIG *cfg) {

	this -> _properties = {
		{ "driver", "''" },
		{ "device", "''" },
		{ "foreground", "'ffffff'" },
		{ "background", "'000000'" },
		{ "basecolor", "'000000'" },
		{ "orientation", "0" },
		{ "backlight", "5" },
	};

	this -> _clean_up = true;
	this -> _orientation = ORIENTATION::rotate0();
	this -> _backlight = 5;

	if ( cfg -> _cfg.contains("variables") && std::holds_alternative<CONFIG::MAP>(cfg -> _cfg["variables"]))
		this -> init_variables(&(std::get<CONFIG::MAP>(cfg -> _cfg["variables"])));

	if ( cfg -> _cfg.contains("display") && std::holds_alternative<CONFIG::MAP>(cfg -> _cfg["display"]))
		this -> init_display(&(std::get<CONFIG::MAP>(cfg -> _cfg["display"])));

	std::string _driver, _device;
	std::string _fg, _bg, _bl;
	int _backlight = 5;

	if ( _driver = this -> P2S("driver"); _driver.empty())
		throws << "display driver is not set" << std::endl;

	if ( auto _p = this -> property["device"]; _p.is_string())
		_device = _p.to_string();

	if ( auto _p = this -> property["orientation"]; _p.is_number()) {

		int _o = _p.to_int();
		if ( _o < 0 || _o > 3 )
			logger::error["display"] << "incorrect value for orientation: " << _o << std::endl;
		else {

			if ( _o == 1 ) this -> _orientation = ORIENTATION::rotate90();
			else if ( _o == 2 ) this -> _orientation = ORIENTATION::rotate180();
			else if ( _o == 3 ) this -> _orientation = ORIENTATION::rotate270();
		}
	}

	if ( _fg = this -> P2S("foreground"); !_fg.empty()) {

		if ( !RGBA::check_color(_fg)) {
			logger::error["display"] << "invalid foreground color '" << _fg << "'" << std::endl;
			_fg = "ffffff";
		}
	} else _fg = "ffffff";

	if ( _bg = this -> P2S("background"); !_bg.empty()) {

		if ( !RGBA::check_color(_bg)) {
			logger::error["display"] << "invalid foreground color '" << _bg << "'" << std::endl;
			_bg = "000000";
		}
	} else _bg = "000000";

	if ( _bl = this -> P2S("basecolor"); !_bl.empty()) {

		if ( !RGBA::check_color(_bg)) {
			logger::error["display"] << "invalid basecolor '" << _bl << "'" << std::endl;
			_bl = "000000";
		}
	} else _bl = "000000";

	_backlight = this -> P2I("backlight", 5);

	if ( _backlight < 0 || _backlight > 10 ) {

		logger::error["display"] << "invalid value for backlight: " <<
			_backlight << ", range is between 0 and 10" << std::endl;
		_backlight = 5;
	}

	RGBA fg = RGBA(_fg, true);
	RGBA bg = RGBA(_bg, true);
	RGBA bl = RGBA(_bl, true);

	RGBA::FG = { .R = fg.R, .G = fg.G, .B = fg.B, .A = fg.A };
	RGBA::BG = { .R = bg.R, .G = bg.G, .B = bg.B, .A = bg.A };
	RGBA::BL = { .R = bl.R, .G = bl.G, .B = bl.B, .A = bl.A };

	this -> _backlight = _backlight;

	try {
		this -> init_driver(_driver, _device);
	} catch ( const std::runtime_error& e ) {
		this -> driver = nullptr;
		throws << e.what() << std::endl;
	}

	if ( this -> driver == nullptr )
		throws << "driver initialization failed, reason: unknown" << std::endl;

	// add plugins
	this -> plugins = new plugin;

	// add timers
	this -> init_timers(&cfg -> _cfg);

	// add widgets
	this -> widgets = new widget;
	this -> init_widgets(&cfg -> _cfg);
	this -> init_layout(&cfg -> _cfg);

	if ( this -> layout == nullptr )
		throws << "fatal error, layout not defined in configuration" << std::endl;

        if ( cfg -> _cfg.contains("scheduler") && std::holds_alternative<CONFIG::MAP>(cfg -> _cfg["scheduler"]))
                this -> init_scheduler(&(std::get<CONFIG::MAP>(cfg -> _cfg["scheduler"])));
	else this -> init_scheduler(nullptr);
}

DISPLAY::~DISPLAY() {

	if ( this -> driver != nullptr ) {

		try {
			display -> page_number();
			this -> driver -> clear();
		} catch ( const std::runtime_error& e ) {}
		delete this -> driver;
		this -> driver = nullptr;
	}

	if ( this -> widgets != nullptr ) {
		delete this -> widgets;
		this -> widgets = nullptr;
	}

	if ( this -> plugins != nullptr ) {
		delete this -> plugins;
		this -> plugins = nullptr;
	}

	this -> timers.clear();

	if ( this -> actions != nullptr ) {
		delete this -> actions;
		this -> actions = nullptr;
	}

	if ( this -> layout != nullptr ) {
		delete this -> layout;
		this -> layout = nullptr;
	}

	if ( this -> scheduler != nullptr ) {
		delete this -> scheduler;
		this -> scheduler = nullptr;
	}
}

int DISPLAY::width() {
	return this -> orientation().isUpsideDown() ? this -> _height : this -> _width;
}

int DISPLAY::height() {
	return this -> orientation().isUpsideDown() ? this -> _width : this -> _height;
}

int DISPLAY::pwidth() {
	return this -> _width;
}

int DISPLAY::pheight() {
	return this -> _height;
}

ORIENTATION DISPLAY::orientation() {
	return this -> _orientation;
}

void DISPLAY::orientation(ORIENTATION orientation) {
	this -> _orientation = orientation;
}

int DISPLAY::backlight() {
	return this -> _backlight;
}

void DISPLAY::backlight(int value) {

	if ( this -> driver != nullptr ) {

		this -> driver -> backlight(value);
		this -> _backlight = this -> driver -> backlight();
	} else this -> _backlight = value;
}

void DISPLAY::clear() {

	if ( this -> driver == nullptr )
		return;

	this -> driver -> reset_canvas();
	this -> driver -> clear();
}

int DISPLAY::page_number() {

	if ( this -> canvas.contains(this -> _page))
		return this -> _page;

	if ( this -> _page != 0 && this -> canvas.contains(0)) {
		logger::warning["display"] << "falling back to page 0" << std::endl;
		this -> _page = 0;
		return this -> _page;
	}

	throws << "failed to retrieve page, currently on " << LAYOUT::page_name(this -> _page) << " but it doesn't exist in layout" << std::endl;
}

bool DISPLAY::page_number(int page_no) {

	if ( this -> _page != page_no && this -> canvas.contains(page_no)) {
		this -> _page = page_no;
		return true;
	}

	if ( this -> _page == page_no )
		logger::warning["display"] << "cannot set page to " << LAYOUT::page_name(page_no) << " as it is already currently active page" << std::endl;
	else logger::error["display"] << "cannot set page to " << LAYOUT::page_name(page_no) << ", it does not exist in layout" << std::endl;

	return false;
}



void DISPLAY::refresh() {

	if ( this -> driver == nullptr || this -> layout == nullptr || this -> widgets == nullptr || this -> widgets -> widgets.empty())
		return;

	this -> driver -> refresh();
}

void DISPLAY::add_pixel(int x, int y, int page, int layer, RGBA color) {

	int _x = x;
	int _y = y;

	if ( this -> _orientation.isRotated90()) {
		_x = y;
		_y = this -> width() - 1 - x;
	} else if ( this -> _orientation.isRotated180()) {
		_x = this -> width() - 1 - x;
		_y = this -> height() - 1 - y;
	} else if ( this -> _orientation.isRotated270()) {
		_x = this -> height() - 1 - y;
		_y = x;
	}

	if ( _x < 0 || _x >= this -> _width || _y < 0 || _y >= this -> _height ) {

		logger::warning["draw"] << "add_pixel: x/y out of bounds" <<
                        logger::detail("(x=" + std::to_string(x) + "->" + std::to_string(_x) +
                                        ", y=" + std::to_string(y) + "->" + std::to_string(_y) +
                                        ", rot=" + std::to_string(this -> _orientation.angle()) +
                                        ", width=" + std::to_string(this -> pwidth()) + "->" + std::to_string(this -> width()) +
                                        ", height=" + std::to_string(this -> pheight()) + "->" + std::to_string(this -> height()) +
					")") << std::endl;
		return;
	}

	if ( !this -> canvas.contains(page))
		throws << "add_pixel: error while adding pixel, " << LAYOUT::page_name(page) << " is not initialized" << std::endl;

	if ( !this -> canvas[page].contains(layer))
		throws << "add_pixel: error while adding pixel, " << LAYOUT::page_name(page) <<
			" does not have layer " << layer << " initialized" << std::endl;

	if ( this -> canvas[page][layer].size() < (size_t)(( _y * this -> pwidth()) + _x + 1 ))
		throws << "add_pixel: pixel coordinates out of canvas bounds" << std::endl;

	this -> canvas[page][layer][(_y * this -> pwidth()) + _x] = color;
}

std::vector<RGBA> DISPLAY::new_layer() {

	std::vector<RGBA> layer;

	for ( int i = 0; i < this -> _width * this -> _height; i++ )
		layer.push_back(RGBA(RGBA::NO));

	return layer;
}

void DISPLAY::init_variables(CONFIG::MAP *cfg) {

	for ( auto& [k, v] : *cfg ) {

		std::string key = k;

		if ( !std::holds_alternative<std::string>(v)) {

			logger::error["config"] << "array and objects are not supported in variables section, ignoring '" <<
				key << "'" << std::endl;
			continue;
		}

		std::string value = std::get<std::string>(v);
		value = common::trim_ws(value);

		if ( !CONFIG::parse_option("variables", key, value, nullptr)) {

			CONFIG::variables[key] = "''";
			continue;
		}

		if ( value.empty()) {

			CONFIG::variables[key] = "''";
			continue;
		}

		expr::expression e(value);

		if ( value.find_first_not_of("1234567890.") != std::string::npos ) { // it is a string?

			try {
				expr::RESULT result = e.evaluate(nullptr, &CONFIG::variables);

				if ( result.operator std::string().empty()) {

					logger::debug["config"] << "note: variable '" << key << "' was set with empty string value after evaluation" << std::endl;
					CONFIG::variables[key] = "''";
				};

				//CONFIG::variables[key] = describe(e);
				CONFIG::variables[key] = common::unquoted(value);
				continue;

			} catch ( std::runtime_error &e ) {

				logger::error["config"] << "variable '" << key << "' evaluation failure: " << e.what() << std::endl;
				CONFIG::variables[key] = "''";
				continue;
			}

			value = common::unquoted(value);

		} else if ( value.find_first_not_of("1234567890") == std::string::npos ) {

			try {
				int i = stoi(value);
				CONFIG::variables[key] = i;
				continue;
			} catch ( const std::exception& e ) {
				CONFIG::variables[key] = value;
				continue;
			}

		} else if ( value.find_first_not_of("1234567890.,") == std::string::npos ) {

			std::string value2 = value;

			while (1) {
				if ( auto pos = value2.find_first_of(','); pos != std::string::npos )
					value2.at(pos) = '.';
				else break;
			}

			for ( auto &ch : value2 )
				if ( ch == ',' ) ch = '.';

			if ( value2.front() == '.' )
				value2 = "0" + value2;

			if ( value2.back() == '.' )
				value2.pop_back();

			size_t c = 0;

			for ( auto& ch : value2 )
				if ( ch == '.' ) c++;

			if ( c == 1 ) {

				try {
					int d = stod(value2);
					CONFIG::variables[key] = d;
					continue;
				} catch ( const std::exception& e ) {
					CONFIG::variables[key] = 0;
					continue;
				}
			} else CONFIG::variables[key] = value;
		} else CONFIG::variables[key] = value;
	}
}

void DISPLAY::init_display(CONFIG::MAP *cfg) {

	std::vector<std::string> allowed_keys = {
		"driver", "device", "foreground", "background", "basecolor", "orientation", "backlight"
	};

	for ( auto& [k, v] : *cfg ) {

		std::string key = k;

		if ( !std::holds_alternative<std::string>(v)) {

			logger::error["config"] << "arrays and objects are not supported in display section, ignoring '" <<
				key << "'" << std::endl;
			continue;
		}

		std::string value = std::get<std::string>(v);

		if ( !CONFIG::parse_option("display", key, value, &allowed_keys))
			continue;
		else if ( key == "driver" ) {

			if ( !CONFIG::evaluate_string("display", key, value, value, true))
				throw std::runtime_error("failed to initialize display, failed to evaluate 'driver'");

			if ( std::find(drv::list.begin(), drv::list.end(), value) == drv::list.end())
				throw std::runtime_error("failed to initialize display, unknown driver '" + value +
					"', available drivers: " + common::join_vector(drv::list));

			this -> _properties[key] = "'" + common::unquoted(value) + "'";

		} else if ( key == "device" ) {

			if ( !CONFIG::evaluate_string("device", key, value, value, false))
				throw std::runtime_error("failed to initialize display, failed to evaluate 'device'");

			this -> _properties[key] = "'" + common::unquoted(value) + "'";

		} else if (( key == "foreground" || key == "background" || key == "basecolor" )) {

			if ( !CONFIG::evaluate_string("display", key, value, value, true)) {

				logger::warning["config"] << "failure with " << key << ( key != "basecolor" ? " color" : "" ) <<
					" in display section, value '" << value << "' failed to evaluate" << std::endl;
				continue;
			}

			if ( !RGBA::check_color(value)) {

				logger::warning["config"] << "failure with " << key << ( key != "basecolor" ? " color " : "" ) <<
					" in display section, value '" << value << "' did not validate as hex color" << std::endl;
				continue;
			}

			this -> _properties[key] = "'" + common::unquoted(value) + "'";

		} else if ( key == "orientation" || key == "backlight" ) {

			int i;

			if ( !CONFIG::evaluate_int("display", key, value, i)) {

				logger::warning["config"] << "failure with " << key << " in display section, value did not" <<
					" evaluate as number" << std::endl;
				continue;
			}

			if ( key == "orientation" && ( i < 0 || i > 3 )) {

				logger::warning["config"] << "failure with " << key << " in display section, value " << i <<
					" not in allowed range between 0 and 3" << std::endl;
				continue;
			} else if ( key == "backlight" && ( i < 0 || i > 10 )) {

				logger::warning["config"] << "failure with " << key << " in display section, value " << i <<
					" not in allowed range between 0 and 10" << std::endl;
				continue;
			}

			this -> _properties[key] = value;

		} else if ( key.empty()) {

			logger::error["config"] << "ignored empty key for display" << std::endl;

		} else if ( std::find(allowed_keys.begin(), allowed_keys.end(), key) == allowed_keys.end()) {

			logger::warning["config"] << "unknown key '" << key << "' for driver, allowed keys are: " <<
				common::join_vector(allowed_keys) << std::endl;
			continue;

		} else if ( value.empty()) {

			logger::debug["verbose"] << "note: '" << key << "' in driver section does not have a value" << std::endl;
		}

	}

}

void DISPLAY::init_timers(CONFIG::MAP *cfg) {

	if ( this -> actions == nullptr )
		this -> actions = new action();

	for ( auto& [k, v] : *cfg ) {

		std::string key = common::unquoted(common::to_lower(common::trim_ws(std::as_const(k))));

		if ( key.starts_with("timer:")) {

			if ( auto pos = key.find_first_of(':'); pos != std::string::npos )
				key = key.erase(0, pos + 1);
			else key = "";

			if ( key.empty()) {

				logger::error["config"] << "syntax error, cannot create timer without name" << std::endl;
				continue;
			}

			if ( !std::holds_alternative<CONFIG::MAP>((*cfg)[k])) {

				logger::error["config"] << "failed to create timer " << ( key.empty() ? "" : ( key + " " )) <<
					", configuration for widget is not object" << std::endl;
				continue;
			}

			logger::debug["config"] << "adding timer: '" << key << "'" << std::endl;

			try {
				this -> timers[key] = TIMER(key, &std::get<CONFIG::MAP>((*cfg)[k]));
			} catch ( std::exception& e ) {
				logger::error["config"] << "failed to add timer '" << key << "', reason: " << e.what() << std::endl;
				if ( this -> timers.contains(key))
					this -> timers.erase(key);
			}
		}
	}
}

void DISPLAY::init_widgets(CONFIG::MAP *cfg) {

	for ( auto& [k, v] : *cfg ) {

		std::string key = common::unquoted(common::to_lower(common::trim_ws(std::as_const(k))));

		if ( key.starts_with("widget:")) {

			if ( auto pos = key.find_first_of(':'); pos != std::string::npos )
				key = key.erase(0, pos + 1);
			else key = "";

			if ( !std::holds_alternative<CONFIG::MAP>((*cfg)[k])) {

				logger::error["config"] << "failed to create widget " << ( key.empty() ? "" : ( key + " " )) <<
					", configuration for widget is not object" << std::endl;
				continue;
			}

			logger::debug["config"] << "adding widget: '" << key << "'" << std::endl;
			this -> widgets -> add(key, &std::get<CONFIG::MAP>((*cfg)[k]));
		}
	}
}

void DISPLAY::init_layout(CONFIG::MAP *cfg) {

	for ( auto& [k, v] : *cfg ) {

		std::string key = common::unquoted(common::to_lower(common::trim_ws(std::as_const(k))));

		if ( key.starts_with("layout:")) {

			logger::error["config"] << "layout section cannot be named, ignored layout section '" <<
				key.erase(0, 7) << "'" << std::endl;
			continue;

		} else if ( key == "layout" && !std::holds_alternative<CONFIG::MAP>((*cfg)[k])) {

			logger::error["config"] << "failed to initialize layout, layout configuration is not object" << std::endl;
			continue;

		} else if ( key == "layout") {

			if ( this -> layout != nullptr ) {

				logger::error["config"] << "duplicate layout section in configuration" << std::endl;
				continue;
			}

			logger::debug["config"] << "adding layout" << std::endl;

			try {
				this -> layout = new LAYOUT(&std::get<CONFIG::MAP>((*cfg)[k]));
			} catch ( const std::exception& e ) {
				logger::error["config"] << "fatal error, failed to initialize layout, reason: " << e.what() << std::endl;
			}
		}
	}

	if ( this -> layout == nullptr )
		return;
}

void DISPLAY::init_driver(const std::string& name, const std::string& device) {

	std::string _name = common::unquoted(common::to_lower(std::as_const(name)));
        std::string _device = common::unquoted(common::to_lower(std::as_const(device)));

        if ( _name.empty())
                throws << "fatal error, valid driver not set in configuration" << std::endl;
        else if ( std::find(drv::list.begin(), drv::list.end(), _name) == drv::list.end())
                throws << "fatal error, unsupported driver '" << _name << "', supported drivers: " <<
                        common::join_vector(drv::list) << std::endl;

        if ( _name == "dpf" && _device.empty())
                throws << "device is not set, dpf driver requires device" << std::endl;

        if ( _name == "dpf" ) {

                try {
                        driver = new drv::DPF(_device, this -> _backlight, this -> _width, this -> _height);
                } catch ( std::runtime_error &e ) {
                        driver = nullptr;
                        throws << "fatal error, reason: " << e.what() << std::endl;
                }
        } else throws << "Unsupported device driver '" << _name << "'" << std::endl;

	if ( this -> driver == nullptr )
		throws << "fatal error, driver " << name << " was not initialized" << std::endl;
}

void DISPLAY::init_canvas() {

	if ( this -> layout != nullptr ) {

		logger::debug["layout"] << "initializing canvas" << std::endl;

		if ( !this -> canvas.empty()) {

			logger::debug["layout"] << "canvas already initialized, re-initializing" << std::endl;
			this -> canvas.clear();
		}

		for ( const auto& page : this -> layout -> pages ) {
			for ( const auto& layer : page.second.layers ) {

				logger::debug["layout"] << "adding layer " << layer.second.number << " to " <<
					LAYOUT::page_name(page.second.number) << " on canvas" << std::endl;

				this -> canvas[page.second.number][layer.second.number] = this -> new_layer();
			}
		}

	} else throws << "failure to initialize canvas, layout not initialized" << std::endl;
}

void DISPLAY::init_scheduler(CONFIG::MAP *cfg) {

	logger::debug["scheduler"] << "initializing scheduler" << std::endl;

	if ( this -> scheduler != nullptr ) {

		logger::warning["scheduler"] << "scheduler already initialized, not going to re-initialize it" << std::endl;
		return;
	}

	bool is_threaded = true;

	if ( cfg != nullptr ) {
		for ( auto& [k, v] : *cfg ) {

			std::string key = common::unquoted(common::to_lower(common::trim_ws(std::as_const(k))));

			if ( key == "threading" || key == "threaded" ) {

				if ( !std::holds_alternative<std::string>(v)) {

					logger::error["config"] << "invalid scheduler config, option " << key << " must be a boolean value" << std::endl;
					continue;
				}

				std::string value = common::trim_ws(common::unquoted(common::to_lower(common::trim_ws(std::as_const(std::get<std::string>(v))))));

				if ( value.empty()) {

					logger::error["config"] << "invalid scheduler config, option " << key << " is empty" << std::endl;
					continue;

				} else if ( !common::is_any_of(value, { "0", "1", "false", "true", "yes", "no" })) {

					logger::error["config"] << "invalid scheduler config, option " << key << " must be a boolean value, not '" <<
						value << "'" << std::endl;
					continue;
				}

				if ( value == "0" || value == "false" || value == "no" )
					is_threaded = false;

				continue;
			} else if ( key.empty()) continue;

			logger::error["config"] << "invalid scheduler configuration, unsupported option " << key << std::endl;
		}
	}

	logger::debug["scheduler"] << ( is_threaded ? "enabling" : "disabling" ) << " threading" << std::endl;

	this -> scheduler = new SCHEDULER(this, is_threaded);
}

void DISPLAY::clean_up() {

	if ( !this -> _clean_up )
		return;

	this -> _clean_up = false;

	if ( this -> layout -> pages.empty())
		throws << "fatal error, layout has no pages" << std::endl;

	for ( auto &p : this -> layout -> pages ) {

		if ( !p.second.on_enter.empty() &&
			!this -> timers.contains(p.second.on_enter))
			this -> layout -> pages[p.second.number].on_enter = "";

		if ( !p.second.on_exit.empty() &&
			!this -> timers.contains(p.second.on_exit))
			this -> layout -> pages[p.second.number].on_exit = "";

	}

	if ( this -> layout -> pages.contains(this -> layout -> default_page))
		this -> _page = this -> layout -> default_page;
	else if ( this -> layout -> pages.contains(0))
		this -> _page = 0;
	else this -> _page = this -> layout -> pages[0].number;

	if ( !this -> layout -> pages[this -> _page].on_enter.empty() &&
		this -> timers.contains(this -> layout -> pages[this -> _page].on_enter)) {

		this -> timers[this -> layout -> pages[this -> _page].on_enter].last_updated = std::chrono::milliseconds(0);
		this -> timers[this -> layout -> pages[this -> _page].on_enter].update();
	}

	for ( const auto& timer : this -> timers )
		this -> timers[timer.first].last_updated =
			std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());

	for ( const auto& page : this -> layout -> pages ) {

		for ( const auto& layer : page.second.layers ) {

			std::vector<std::string> missing_widgets;

			for ( const auto& widget : layer.second.widgets ) {

				if ( !this -> widgets -> widgets.contains(widget.name) &&
					std::find(missing_widgets.begin(), missing_widgets.end(), widget.name) ==
					missing_widgets.end()) {

					missing_widgets.push_back(widget.name);
				}
			}

			if ( !missing_widgets.empty()) {

				logger::vverbose["config"] << "remove missing " << ( missing_widgets.size() == 1 ? "widget " : "widgets " ) <<
					common::join_vector(missing_widgets) <<
					" from layer " << layer.second.number << " on " << LAYOUT::page_name(page.second.number) << std::endl;

				for ( const std::string& widget : missing_widgets ) {

					this -> layout -> pages[page.second.number].layers[layer.second.number].widgets.erase(std::remove_if(
						this -> layout -> pages[page.second.number].layers[layer.second.number].widgets.begin(),
						this -> layout -> pages[page.second.number].layers[layer.second.number].widgets.end(),
						[&](LAYOUT::WIDGET_LINK const& w) {
							return w.name == widget;
						}), this -> layout -> pages[page.second.number].layers[layer.second.number].widgets.end());
				}
			}
		}

		std::vector<int> empty_layers;

		for ( const auto& layer : page.second.layers ) {

			if ( layer.second.widgets.empty() &&
				std::find(empty_layers.begin(), empty_layers.end(), layer.second.number) == empty_layers.end()) {

				empty_layers.push_back(layer.second.number);
			}
		}

		if ( !empty_layers.empty()) {

			std::vector<std::string> s_empty_layers;

			for ( const int& l : empty_layers )
				s_empty_layers.push_back(std::to_string(l));

			logger::vverbose["config"] << "removing empty " <<
				( empty_layers.size() == 1 ? "layer " : "layers (" ) <<
				common::join_vector(s_empty_layers) <<
				( empty_layers.size() == 1 ? "" : ")" ) << " from " <<
				LAYOUT::page_name(page.second.number) << std::endl;
		}

		for ( const int& l : empty_layers )
			this -> layout -> pages[page.second.number].layers.erase(l);
	}
}

bool DISPLAY::setpage(int page_no) {

	if ( this -> scheduler == nullptr || this -> layout == nullptr || !this -> layout -> pages.contains(page_no))
		return false;

	if ( page_no == this -> _page ) {

		logger::warning["display"] << "cannot switch to " << LAYOUT::page_name(page_no) << ", page is already active" << std::endl;
		return false;
	}

	int current_page = this -> _page;

	if ( this -> layout -> pages.contains(this -> _page) && !this -> layout -> pages[this -> _page].on_exit.empty())
		this -> layout -> pages[this -> _page].exit();

	if ( !this -> page_number(page_no) || this -> _page == current_page ) { // switches this -> _page

		logger::error["display"] << "error occurred while trying to switch to " << LAYOUT::page_name(page_no) << std::endl;
		this -> _page = current_page;
		return false;
	}

	if ( page_no != -1 && this -> scheduler -> exit_loop()) return true;

	if ( this -> layout -> pages.contains(this -> _page) && !this -> layout -> pages[this -> _page].on_enter.empty())
		this -> layout -> pages[this -> _page].enter();

	if ( page_no != -1 && this -> scheduler -> exit_loop()) return true;

	this -> layout -> pages[this -> _page].update_widgets();

	if ( page_no != -1 && this -> scheduler -> exit_loop()) return true;

	this -> layout -> render(&this -> _page);

	if ( page_no != -1 && this -> scheduler -> exit_loop()) return true;

	this -> refresh();
	return true;
}

bool DISPLAY::goodbye() {

	int page_no = -1;

	if ( !this -> setpage(page_no))
		return false;

	if ( !this -> layout -> pages[page_no].on_exit.empty() &&
			this-> timers.contains(this -> layout -> pages[page_no].on_exit)) {

		this -> layout -> pages[page_no].exit();
		this -> layout -> render(&page_no);
		this -> refresh();
        }

	return true;
}

bool DISPLAY::threading() {

	return this -> scheduler == nullptr ? false : this -> scheduler -> threading();
}

void DISPLAY::run() {

	this -> clean_up();

	if ( this -> scheduler != nullptr )
		this -> scheduler -> run();
	else throws << "fatal error, scheduler is not ready" << std::endl;
}

std::ostream& operator <<(std::ostream& os, DISPLAY& d) {

	d.clean_up();

	os << "variables {\n";

	for ( const auto& [k, v] : CONFIG::variables ) {
		os << common::padding(2) << k << common::padding(20 - k.size());
		if ( v.is_string())
			os << "'" << v << "'" << "\n";
		else os << v << "\n";
	}

	os << "}\n\ndisplay {\n";
	for ( const auto& [k, v] : d._properties )
		os << common::padding(2) << k << common::padding(20 - k.size()) << v << "\n";

	os << "}\n";

	for ( const auto& [k, timer] : d.timers )
		os << ( timer.name() == d.timers.front().second.name() ? "\n" : "\n\n" ) << timer;

	if ( !d.timers.empty())
		os << "\n";

	if ( !d.widgets -> empty())
		os << "\n" << d.widgets << "\n";

	return os;
}

std::ostream& operator <<(std::ostream& os, DISPLAY *d) {

	os << *d;
	return os;
}
