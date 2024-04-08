#include <iostream>
#include <stdexcept>
#include <algorithm>
#include <vector>

#include "common.hpp"
#include "logger.hpp"
#include "throws.hpp"

#include "config.hpp"

#include "plugin_classes.hpp"
#include "widget_classes.hpp"
#include "expr/expression.hpp"
#include "display.hpp"
#include "timer.hpp"
#include "layout.hpp"

expr::VARIABLE LAYOUT::fn_page(const expr::FUNCTION_ARGS& args) {

	if ( !args.empty())
		logger::warning["plugin"] << "page function does not accept arguments" << std::endl;

	if ( display == nullptr )
		return (double)-1;
	else return (double)display -> page_number();
}

std::string LAYOUT::page_name(const int& page_no) {

	if ( page_no == -1 )
		return "goodbye page";
	else return "page " + std::to_string(page_no);

}

static bool parse_coords(const std::string& value, int& x, int& y) {

	std::string _value = common::unquoted(common::to_lower(common::trim_ws(std::as_const(value))));

	if ( _value.empty() || ( _value.front() != 'x' && !std::isdigit(_value.front())))
		return false;

	while (1) {

		if( auto pos = _value.find_first_of('.'); pos != std::string::npos )
			_value.at(pos) = ',';
		else break;
	}

	while (1) {

		if ( auto pos = _value.find_first_of(common::whitespace); pos != std::string::npos)
			_value.erase(pos, 1);
		else break;
	}

	if ( _value.front() == 'x' && _value.size() > 1 && std::isdigit(_value.at(1))) { // convert old format to new format

		if ( auto pos1 = _value.find_first_of('x'); pos1 != std::string::npos )
			if ( auto pos2 = _value.find_first_of(','); pos2 == std::string::npos )
				if ( auto pos3 = _value.find_first_of('y'); pos3 != std::string::npos )
					_value.insert(pos3, ","); // comma is missing, add it

		size_t c1 = 0;
		size_t c2 = 0;
		size_t c3 = 0;

		for ( auto& ch : _value ) {

			if ( ch == ',' ) c1++;
			else if ( ch == 'x' ) c2++;
			else if ( c1 == 1 && ch == 'y' ) c3++;
		}

		if ( c1 == 1 && c2 == 1 && c3 == 1 ) {

			while (1) {

				if ( auto pos = _value.find_first_of('x'); pos != std::string::npos )
					_value.erase(pos, 1);
				else if ( auto pos = _value.find_first_of('y'); pos != std::string::npos )
					_value.erase(pos, 1);
				else break;
			}
		}
	}

	if ( _value.find_first_not_of("1234567890,") != std::string::npos || !std::isdigit(_value.front()))
		return false;

	size_t c = 0;

	for ( auto& ch : _value )
		if ( ch == ',' ) c++;

	if ( c != 1 )
		return false;

	if ( auto pos = _value.find_first_of(','); pos != std::string::npos ) {

		std::string sx = _value.substr(0, pos);
		std::string sy = _value;
		sy.erase(0, pos);
		while ( sx.back() == ',' ) sx.pop_back();
		while ( sy.front() == ',' ) sy.erase(0, 1);
		sx = common::trim_ws(sx);
		sy = common::trim_ws(sy);

		if ( sx.empty() || sy.empty() ||
			sx.find_first_not_of("1234567890") != std::string::npos ||
			sy.find_first_not_of("1234567890") != std::string::npos )
			return false;

		int _x, _y;

		try {
			_x = std::stoi(sx);
			_y = std::stoi(sy);
			x = _x;
			y = _y;
			return true;
		} catch ( const std::exception& e ) {
			return false;
		}
	}

	return false;
}

std::vector<int> LAYOUT::parse_page_sequence(const std::string& s) {

	std::string _s = common::unquoted_and_trimmed(s);

	for ( auto& ch : _s )
		if ( common::whitespace.find_first_of(ch) != std::string::npos )
			ch = ' ';

	if ( _s.find_first_not_of("- ,1234567890") != std::string::npos ) {

		logger::error["config"] << "syntax error for page sequence in layout config, value '" << s << "' is not a valid list of page numbers" << std::endl;
		return {};
	}

	if ( _s.empty()) {

		logger::error["config"] << "syntax error for page sequence in layout config, list of page numbers is empty" << std::endl;
		return {};
	}

	std::vector<int> vec;
	std::string arg;
	for ( const auto& ch : _s ) {

		if ( std::isdigit(ch)) {
			arg += ch;
			continue;
		} else if ( ch == '-' && arg.empty()) {
			arg += ch;
			continue;
		} else if ( ch == '-' && !arg.empty()) {

			logger::error["config"] << "syntax error for page sequence, invalid - symbol between numeric characters, aborting sequence parsing" << std::endl;
			return {};

		} else if (( ch == ' ' || ch == ',' ) && arg.empty()) {
			continue;
		} else if (( ch == ' ' || ch == ',' )) {

			try {
				int i = std::stoi(arg);

				if ( i < 0 ) {

					logger::error["config"] << "negative numbers in page sequence are not allowed, replacing " << i <<
						" with " << std::abs(i) << std::endl;
					i = std::abs(i);
				}

				if ( !vec.empty() && vec.back() == i)
					logger::error["config"] << "cannot add same page numbers in row to page sequence, ignoring second " << i << std::endl;
				else if ( !this -> pages.contains(i))
					logger::error["config"] << "unable to add page " << i << " to page sequence, page does not exist in layout" << std::endl;
				else vec.push_back(i);

			} catch ( const std::exception& e ) {

				logger::error["config"] << "failed to parse '" << arg << "' to number, value ignored" << std::endl;
			}

			arg = "";
		}
	}

	if ( !arg.empty()) {

		try {
			int i = std::stoi(arg);

			if ( i < 0 ) {

				logger::error["config"] << "negative numbers in page sequence are not allowed, replacing " << i <<
                                                " with " << std::abs(i) << std::endl;
				i = std::abs(i);
			}

			if ( !vec.empty() && vec.back() == i)
				logger::error["config"] << "cannot add same page numbers in row to page sequence, ignoring second " << i << std::endl;
			else if ( !this -> pages.contains(i))
				logger::error["config"] << "unable to add page " << i << " to page sequence, page does not exist in layout" << std::endl;
			else vec.push_back(i);

		} catch ( const std::exception& e ) {

			logger::error["config"] << "failed to parse '" << arg << "' to number, value ignored" << std::endl;
		}
	}

	if ( vec.size() > 1 && ( vec.front() == vec.back() || vec[vec.size() - 2] == vec.back())) {

		logger::error["config"] << "invalid page sequence configuration, first and last pages cannot be same" << std::endl;

		while ( vec.size() > 1 && ( vec.front() == vec.back() || vec[vec.size() - 2] == vec.back()))
			vec.pop_back();

		if ( vec.size() < 2 ) {

			logger::error["config"] << "page sequence invalidated, page sequence is empty after validation" << std::endl;
			return {};
		}
	}

	if ( vec.size() < 2 ) {

		logger::error["config"] << "invalid page sequence, page sequence must contain atleast 2 different page numbers" << std::endl;
		return {};
	}

	return vec;
}

LAYOUT::WIDGET_LINK::WIDGET_LINK(const std::string& key, const std::string& value) {

	if ( parse_coords(value, this -> x, this -> y)) {
		this -> name = common::unquoted(common::to_lower(common::trim_ws(std::as_const(key))));
	} else if ( parse_coords(key, this -> x, this -> y)) {
		this -> name = common::unquoted(common::to_lower(common::trim_ws(std::as_const(value))));
	} else throws << "could not parse coordinates for widget" << std::endl;

}

widget::WIDGET* LAYOUT::WIDGET_LINK::get_ptr() {

	if ( !display -> widgets -> contains(this -> name))
		return nullptr;

	return display -> widgets -> widgets[this -> name].get();
}

bool LAYOUT::WIDGET_LINK::reloads() {

	if ( !display -> widgets -> contains(this -> name))
		return false;

	auto *w = display -> widgets -> widgets[this -> name].get();
	return w -> reloads();
}

bool LAYOUT::WIDGET_LINK::update() {

	if ( !display -> widgets -> contains(this -> name))
		return false;

	auto *w = display -> widgets -> widgets[this -> name].get();
	return w -> update();
}

void LAYOUT::PAGE::update_widgets() {

	for ( auto& [k, layer ] : this -> layers )
		for ( auto& widget : layer.widgets )
			widget.update();
}

bool LAYOUT::PAGE::enter() {

	if ( this -> on_enter.empty())
		return true;

	if ( !display -> timers.contains(this -> on_enter)) {

		logger::error["layout"] << "on_enter timer '" << this -> on_enter << "' has failed, timer does not exist" << std::endl;
		this -> on_enter = "";
		return false;
	}

	display -> timers[this -> on_enter].last_updated = std::chrono::milliseconds(0);
	display -> timers[this -> on_enter].update();
	return true;
}

bool LAYOUT::PAGE::exit() {

	if ( this -> on_exit.empty())
		return true;

	if ( !display -> timers.contains(this -> on_exit)) {

		logger::error["layout"] << "on_exit timer '" << this -> on_exit << "' has failed, timer does not exist" << std::endl;
		this -> on_exit = "";
		return false;
	}

	display -> timers[this -> on_exit].last_updated = std::chrono::milliseconds(0);
	display -> timers[this -> on_exit].update();
	return true;
}

LAYOUT::LAYOUT() {

	this -> default_page = 0;
	this -> pages.clear();

	CONFIG::functions.append({ "page", LAYOUT::fn_page });
	CONFIG::functions.append({ "getpage", LAYOUT::fn_page });
}

LAYOUT::LAYOUT(CONFIG::MAP *cfg) {

	this -> default_page = 0;
	std::string new_page_sequence = "";

	for ( auto& [k, v] : *cfg ) {

		std::string key = common::unquoted(common::to_lower(common::trim_ws(std::as_const(k))));

		if ( key == "default_page" ) key = "default";

		if ( key.empty()) {

			logger::error["config"] << "syntax error, empty key in layout section" << std::endl;
			continue;

		} else if ( key.starts_with("layer")) {

			logger::error["config"] << "syntax error, '" << key << "', layers must be placed inside pages" << std::endl;
			continue;

		} else if ( key != "timers" && key != "default" && key != "sequence" && !key.starts_with("page") && key != "goodbye" ) {

			logger::error["config"] << "syntax error, unknown key '" << key << "' for section layout, ignoring" << std::endl;
			continue;
		}

		if ( key == "default" && std::holds_alternative<std::string>(v)) {

			std::string def_s = common::trim_ws(common::unquoted(common::trim_ws(std::as_const(std::get<std::string>(v)))));

			if ( def_s.front() == '-' && def_s.size() > 1 && def_s.substr(1, def_s.size() - 1).find_first_not_of("1234567890") == std::string::npos ) {

				def_s.erase(0, 1);

				logger::error["config"] << "default page -" << def_s << " is not valid setting, negative values not accepted; " <<
								"negative sign removed" << std::endl;
			}

			if ( def_s.empty()) {

				logger::error["config"] << "syntax error, layout's default page value is empty" << std::endl;

			} else if ( def_s.find_first_not_of("1234567890") != std::string::npos ) {

				logger::error["config"] << "syntax error, layout's default page's value '" << def_s << "' is not a number" << std::endl;

			} else {

				try {

					this -> default_page = std::stoi(def_s);

				} catch ( const std::exception& e ) {

					this -> default_page = 0;
					logger::error["config"] << "default page number failed to convert '" << def_s <<
						"' to number, reason:" << e.what() << std::endl;
				}
			}

			continue;

		} else if ( key == "default" && !std::holds_alternative<std::string>(v)) {

			logger::error["config"] << "syntax error, default page must be a number, not array or object" << std::endl;
			continue;

		} else if ( key == "sequence" && std::holds_alternative<std::string>(v)) {

			if ( !new_page_sequence.empty())
				logger::error["config"] << "duplicate definition for page sequence in layout config, new sequence is ignored" << std::endl;
			else new_page_sequence = std::get<std::string>(v);
			continue;

		} else if ( key == "sequence" && !std::holds_alternative<std::string>(v)) {

			logger::error["config"] << "syntax error, page sequence in section layout, must be a comma separated list of numbers, not array or object" << std::endl;
			continue;

		} else if ( key == "timers" && !std::holds_alternative<CONFIG::VECTOR>(v)) {

			logger::error["config"] << "syntax error, timers in section layout, must be array" << std::endl;
			continue;

		} else if ( key == "timers" && !this -> timers.empty()) {

			logger::error["config"] << "duplicate timers configuration in layout root level, ignored" << std::endl;
			continue;

		} else if ( key == "timers" ) {

			this -> timers = std::get<CONFIG::VECTOR>(v);
			this -> timers.erase(std::unique( this -> timers.begin(), this -> timers.end()), this -> timers.end());
			continue;

		} else if (( key.starts_with("page ") || key.starts_with("page:")) && !std::holds_alternative<CONFIG::MAP>(v)) {

			std::string page_s = key.erase(0, 5);
			logger::error["config"] << "page in layout config must be object, ignoring page '" <<
				page_s << "'" << std::endl;
			continue;

		} else if ( key.starts_with("page:") || key == "goodbye" ) {

			// get page attributes
			int page_no;

			if ( key == "goodbye" ) {

				if ( this -> pages.contains(-1)) {

					logger::error["config"] << "ignoring duplicate goodbye page" << std::endl;
					continue;
				}

				page_no = -1;

			} else {

				std::string page_s = key.erase(0, 5);

				try {
					page_no = std::stoi(page_s);
				} catch ( const std::exception &e ) {

					logger::error["config"] << "cannot parse page number from '" << page_s << "' in layout configuration, reason: " << e.what() << std::endl;
					continue;
				}

				if ( page_no < 0 ) {

					logger::error["config"] << "cannot create page " << page_no << ", page numbers must be positive numbers, creating page " <<
									std::abs(page_no) << " instead" << std::endl;

					page_no = std::abs(page_no);
				}

				if ( this -> pages.contains(page_no)) {

					logger::error["config"] << "duplicate page number " << page_no << " in layout configuration, ignored" << std::endl;
					continue;
				}
			}

			// begin page setup
			this -> pages[page_no] = LAYOUT::PAGE(page_no);

			for ( auto& [k2, v2] : std::get<CONFIG::MAP>(v)) {

				std::string key2 = common::unquoted(common::to_lower(common::trim_ws(std::as_const(k2))));

				if ( key2.empty()) {

					logger::error["config"] << "syntax error, empty key in layout section on " << page_name(page_no) << std::endl;

					continue;

				} else if ( page_no == -1 && key == "timers" ) {

					logger::error["config"] << "syntax error, goodbye page does not support timers" << std::endl;
					continue;

				} else if ( key2 != "timers" && key2 != "on_enter" && key2 != "on_exit" && !key2.starts_with("layer")) {

					logger::error["config"] << "syntax error in page configuration, unknown key '" << key2 << "'" << std::endl;
					continue;

				} else if (( key2 == "on_enter" || key2 == "on_exit" ) && !std::holds_alternative<std::string>(v2)) {

					logger::error["config"] << "syntax error, " << key2 << " on page configuration cannot be array or object vaulue" << std::endl;
					continue;

				} else if (( key2 == "on_enter" || key2 == "on_exit" ) && std::holds_alternative<std::string>(v2)) {

					std::string timer_s = common::trim_ws(common::unquoted(common::trim_ws(std::as_const(std::get<std::string>(v2)))));

					if ( timer_s.empty() && key2 == "on_enter" ) this -> pages[page_no].on_enter = "";
					else if ( timer_s.empty() && key2 == "on_exit" ) this -> pages[page_no].on_exit = "";
					else if ( key2 == "on_enter" ) this -> pages[page_no].on_enter = timer_s;
					else if ( key2 == "on_exit" ) this -> pages[page_no].on_exit = timer_s;
					else logger::error["config"] << "unknown error while setting value '" << timer_s << "' to " <<
						key2 << " on " << page_name(page_no) << std::endl;

					continue;

				} else if ( key2 == "timers" && !std::holds_alternative<CONFIG::VECTOR>(v2)) {

					logger::error["config"] << "syntax error, timers in section layout, must be array" << std::endl;
					continue;

				} else if ( key2 == "timers" && !this -> pages[page_no].timers.empty()) {

					logger::error["config"] << "duplicate timers configuration on " << page_name(page_no) << ", ignored" << std::endl;
					continue;

				} else if ( key2 == "timers" ) {

					this -> pages[page_no].timers = std::get<CONFIG::VECTOR>(v2);
					this -> timers.erase(std::unique(this -> timers.begin(), this -> timers.end()), this -> timers.end());
					continue;

				} else if ( key2.starts_with("layer:") && !std::holds_alternative<CONFIG::MAP>(v2)) {

					logger::error["config"] << "layer in layout's page config must be object, ignoring " << page_name(page_no) << std::endl;
					continue;

				} else if ( key2.starts_with("layer:")) {

					// get layer attributes
					int layer_no;
					std::string layer_s = key2.erase(0, 6);

					try {
						layer_no = std::stoi(layer_s);
					} catch ( const std::exception &e ) {

						logger::error["config"] << "cannot parse layer number from '" << layer_s <<
							"' in layout configuration, reason: " << e.what() << std::endl;
						continue;
					}

					if ( this -> pages[page_no].layers.contains(layer_no)) {

						logger::error["config"] << "duplicate layer number " << layer_no <<
							" in layout configuration on " << page_name(page_no) << ", ignored" << std::endl;
						continue;

					} else if ( !std::holds_alternative<CONFIG::MAP>(v2)) {

						logger::error["config"] << "layer in layout config must be object, ignoring layout " <<
							layer_no << " on " << page_name(page_no) << std::endl;
						continue;

					} else if ( std::holds_alternative<CONFIG::MAP>(v2)) {

						// begin layer setup
						this -> pages[page_no].layers[layer_no] = LAYOUT::LAYER(layer_no);

						for ( auto& [k3, v3] : std::get<CONFIG::MAP>(v2)) {

							std::string key3 = common::unquoted(common::to_lower(common::trim_ws(std::as_const(k3))));

							if ( key3.empty()) {

								logger::error["config"] << "syntax error, empty key in layout section on layer " <<
									layer_no << " of " << page_name(page_no) << std::endl;
								continue;

							} else if ( !std::holds_alternative<std::string>(v3)) {

								logger::error["config"] << "invalid widget configuration on layer " << layer_no <<
									"on " << page_name(page_no) << "; arrays are not supported" << std::endl;

								continue;

							} else if ( std::holds_alternative<std::string>(v3)) {

								try {

									LAYOUT::WIDGET_LINK wl = LAYOUT::WIDGET_LINK(key3, std::get<std::string>(v3));
									this -> pages[page_no].layers[layer_no].widgets.push_back(wl);

								} catch ( const std::exception& e ) {

									logger::error["config"] << "failed to add widget '" << key3 << "' to layer " <<
										layer_no << " on " << page_name(page_no) << std::endl;
									logger::error["config"] << "widget add failure, reason: " << e.what() << std::endl;
								}

								continue;

							} else {

								logger::error["config"] << "invalid widget configuration on " << page_name(page_no) <<
											"for layer " << layer_no << ", arrays are not supported" << std::endl;
								continue;
							}
						}
					}
					// layer setup ends

				} else if ( key2.starts_with("layer")) {

					logger::error["config"] << "syntax error, layer in layout section without required layout number argument" << std::endl;
					continue;

				} else {

					logger::error["config"] << "syntax error, unknown key '" << key2 << "' on " << page_name(page_no) << std::endl;
					continue;
				}
			}
			// end page setup

		} else if ( key.starts_with("page")) {

			logger::error["config"] << "syntax error, page in layout section without required page number argument" << std::endl;
			continue;
		}

	}

	// do some cleanup, remove pages without layers

	{
		std::vector<int> pages_to_remove;
		for ( const auto& [k, v] : this -> pages ) {

			if ( k < 0 )
				continue;

			if ( this -> pages[k].layers.empty() && std::find(pages_to_remove.begin(), pages_to_remove.end(), k) == pages_to_remove.end())
				pages_to_remove.push_back(k);

		}

		if ( pages_to_remove.size() > 1 )
			std::sort(pages_to_remove.begin(), pages_to_remove.end(), [](int a, int b) { return a < b; });

		for ( const int& p : pages_to_remove ) {

			logger::warning["config"] << "removing page " << p << " from layout, page does not have a layer" << std::endl;
			this -> pages.erase(p);
		}
	}

	// do some cleanup, remove timers from pages if they are already on global level

	for ( const auto& page : this -> pages ) {

		size_t old_size = this -> pages[page.second.number].timers.size();

		this -> pages[page.second.number].timers.erase(std::remove_if(
			this -> pages[page.second.number].timers.begin(),
			this -> pages[page.second.number].timers.end(),
			[&](const std::string& t) { return std::find(this -> timers.begin(), this -> timers.end(), t) != this -> timers.end(); }),
				this -> pages[page.second.number].timers.end());

		size_t new_size = this -> pages[page.second.number].timers.size();

		if ( old_size - new_size > 0 )
			logger::vverbose["config"] << "removed " << ( old_size - new_size ) << " timers that were already in global level" << std::endl;

	}

	if ( this -> pages.empty() || ( this -> pages.contains(-1) && this -> pages.size() == 1 ))
		throws << "fatal error, layout has no pages" << std::endl;

	if ( !this -> pages.contains(this -> default_page)) {

		int new_default = -1;

		for ( const auto& [k, v] : this -> pages ) {

			if ( k >= 0 ) {
				new_default = k;
				break;
			}
		}

		if ( new_default < 0 || new_default == this -> default_page )
			throws << "fatal error, unable to set default page to page that actually exists in layout" << std::endl;

		logger::error["config"] << "default page " << this -> default_page << " does not exist in layout, setting page " <<
						new_default << " as default page" << std::endl;

		this -> default_page = this -> pages[0].number;
	}

	// validate page sequence
	if ( !new_page_sequence.empty()) {

		this -> page_sequence = this -> parse_page_sequence(new_page_sequence);

		if ( this -> page_sequence.size() > 1) {

			std::string s;
			for ( const auto& i : this -> page_sequence )
				s += ( s.empty() ? "" : ", " ) + std::to_string(i);

			logger::info["config"] << "adding page sequence: " << s << std::endl;

		} else this -> page_sequence.clear();
	}

	// validate goodbye page

	if ( this -> pages.contains(-1) && this -> pages[-1].layers.empty()) {

		logger::error["config"] << "goodbye page does not have any layers, removing goodbye page" << std::endl;
		this -> pages.erase(-1);

	} else if ( this -> pages.contains(-1)) {

		logger::info["config"] << "adding goodbye layout" << std::endl;
	}

	if ( !this -> page_sequence.empty()) {

		this -> prev_page_index = this -> page_sequence.size() - 1;
		this -> next_page_index = 0;

		logger::vverbose["config"] << "previous page in sequence is " << LAYOUT::page_name(this -> page_sequence[this -> prev_page_index]) << std::endl;
		logger::vverbose["config"] << "next page in sequence is " << LAYOUT::page_name(this -> page_sequence[this -> next_page_index]) << std::endl;
	}

	CONFIG::functions.append({ "page", LAYOUT::fn_page });
	CONFIG::functions.append({ "getpage", LAYOUT::fn_page });
}

LAYOUT::~LAYOUT() {

	CONFIG::functions.erase("page");
	CONFIG::functions.erase("getpage");
}

const std::string LAYOUT::dump() {

	std::stringstream ss;
	ss << "layout {\n";

	ss << "\n\tdefault " << this -> default_page << "\n";

	for ( const auto& page : this -> pages ) {

		ss << "\n\tpage " << page.second.number << " {" <<
			( display -> _page == page.second.number ? " # active" : "" ) << "\n";

		if ( !page.second.on_enter.empty() || !page.second.on_enter.empty())
			ss << "\n";

		if ( !page.second.on_enter.empty())
			ss << "\t\ton_enter " << page.second.on_enter << "\n";
		if ( !page.second.on_exit.empty())
			ss << "\t\ton_exit  " << page.second.on_exit << "\n";

		for ( const auto& layer : page.second.layers ) {

			ss << "\n\t\tlayer " << layer.second.number << " {" << "\n";

			for ( const auto& widget : layer.second.widgets )
				ss << "\t\t\t" << widget.name << "\t" << widget.x << ", " << widget.y << "\n";

			ss << "\t\t}" << "\n";
		}

		if ( !page.second.timers.empty()) {

			ss << "\n\t\ttimers {" << "\n";
			for ( const std::string& t : page.second.timers )
				ss << "\t\t\t" << t << "\n";
			ss << "\t\t}" << "\n";
		}

		ss << "\t}" << "\n";
	}

	if ( !this -> timers.empty()) {

		ss << "\n\ttimers {" << "\n";
		for ( std::string& t : this -> timers )
			ss << "\t\t" << t << "\n";
		ss << "\t}" << "\n";
	}

	ss << "}";

	return ss.str();
}

void LAYOUT::render(int* forced_page, bool all) {

	for ( auto& [page_no, page] : this -> pages ) {

		if ( !all && (
			( forced_page != nullptr && *forced_page != page_no ) ||
			( forced_page == nullptr && display -> _page != page_no )))
			continue;

		for ( auto& [layer_no, layer] : this -> pages[page_no].layers ) {

			std::vector<std::string> missing_widgets;

			for ( auto& widget : layer.widgets ) {

				if ( !display -> widgets -> contains(widget.name)) {

					if ( std::find(missing_widgets.begin(), missing_widgets.end(), widget.name) ==
						missing_widgets.end()) {

						logger::error["layout"] << "failed to render widget '" << widget.name <<
							"', widget not initialized" << std::endl;

						missing_widgets.push_back(widget.name);
					}
					continue;
				}

				// clear out previous widget bitmap, as size might have changed..
				auto *w = display -> widgets -> widgets[widget.name].get();

				if ( w == nullptr && std::find(missing_widgets.begin(), missing_widgets.end(), widget.name) ==
					missing_widgets.end()) {

					missing_widgets.push_back(widget.name);
					continue;
				}

				for ( int y = 0; y < w -> previous_height(); y++ )
					for ( int x = 0; x < w -> previous_width(); x++ )
						display -> add_pixel(widget.x + x, widget.y + y, page_no, layer_no,
							RGBA(RGBA::NO));

				// draw bitmap
				for ( int y = 0; y < w -> height(); y++ )
					for ( int x = 0; x < w -> width(); x++ ) {

						try {
							display -> add_pixel(widget.x + x, widget.y + y,
							page_no, layer_no, w -> bitmap[(y * w -> width()) + x]);

						} catch (const std::runtime_error& e) {

							logger::warning["render"] << e.what() << std::endl;
						}
					}
			}

			for ( std::string widget : missing_widgets ) {

				layer.widgets.erase(
					std::remove_if(layer.widgets.begin(), layer.widgets.end(), [&](LAYOUT::WIDGET_LINK const& w) {
						return w.name == widget;
					}), layer.widgets.end());
			}

		}
	}

}
