#include <iostream>
#include <memory>

#include "common.hpp"
#include "logger.hpp"
#include "widget_classes.hpp"

std::vector<std::string> widget::types = {
	"image", "ttf", "linechart", "curvechart", "bar", "gauge", "clock"
};

widget::WIDGET::WIDGET() {
}

widget::WIDGET::~WIDGET() {
	this -> bitmap.clear();
}

const std::string widget::WIDGET::name() const {
	return this -> _name;
}

bool widget::WIDGET::reloads() {

	return this -> P2B("reload", false);
}

// When use_cycles is active, 'interval' is a render-cycle count rather than
// milliseconds. The widget updates every N scheduler cycles instead of every N ms.
bool widget::WIDGET::use_cycles() {

	if ( this -> _use_cycles > -1 )
		return (bool)this -> _use_cycles;

	if ( !this -> reloads()) {

		this -> _use_cycles = 0;

	} else if ( this -> P2I("use_cycles", 0) > 0 ) {

		this -> _use_cycles = 1;

	} else this -> _use_cycles = 0;

	if ( this -> P2I("interval", 0) <= 0 )
		this -> _use_cycles = 0;

	this -> _properties["use_cycles"] = std::to_string(this -> _use_cycles);

	logger::debug["widget"] << this -> _name << ": use_cycles=" << (bool)this -> _use_cycles
		<< " reloads=" << this -> reloads() << std::endl;

	return (bool)this -> _use_cycles;
}

int widget::WIDGET::interval() {

	if ( this -> _interval > -1 )
		return this -> _interval;

	if ( !this -> reloads()) {

		this -> _interval = 0;

	} else if ( this -> P2I("use_cycles", 0) == 1 ) {

		int i = this -> P2I("interval", 1);
		if ( i < 1 ) i = 1;
		else if ( i > 100 ) i = 100;
		this -> _interval = i;

	} else {

		int i = this -> P2I("interval", 1500);
		this -> _interval = i < 50 ? 50 : i;

	}

	this -> _properties["interval"] = std::to_string(this -> _interval);

	logger::debug["widget"] << this -> _name << ": interval=" << this -> _interval
		<< (this -> use_cycles() ? "cycles" : "ms") << std::endl;

	return this -> _interval;
}

int widget::WIDGET::width() const {
	return this -> _width;
}

int widget::WIDGET::height() const {
	return this -> _height;
}

int widget::WIDGET::previous_width() const {
	return this -> _pwidth;
}

int widget::WIDGET::previous_height() const {
	return this -> _pheight;
}

bool widget::WIDGET::center() {

	return this -> P2B("center", false);
}

bool widget::WIDGET::visible() {

	return this -> P2B("visible", true);
}

bool widget::WIDGET::needs_draw() const {
	return this -> _needs_draw;
}

unsigned char widget::convert_alpha(unsigned char gdAlpha) {
	return gdAlpha == 127 ? 0 : ( 255 - 2 * gdAlpha );
}

// Returns true when the widget's update interval has elapsed (ms) or its
// render-cycle counter has counted down to zero.
bool widget::WIDGET::time_to_update() {

	if ( this -> use_cycles()) {

		if ( --this -> _cycle != 0 ) {

			if ( this -> _cycle < 0 )
				this -> _cycle = this -> interval();

			logger::debug["widget"] << this -> _name << ": cycle " << this -> _cycle
				<< "/" << this -> interval() << " (not yet)" << std::endl;
			return false;

		} else {
			this -> _cycle = this -> interval();
			logger::debug["widget"] << this -> _name << ": cycle reached, resetting to "
				<< this -> interval() << std::endl;
		}

	} else {

		auto now  = std::chrono::duration_cast<std::chrono::milliseconds>(
			std::chrono::system_clock::now().time_since_epoch());
		auto next = this -> last_updated + std::chrono::milliseconds(this -> interval());

		if ( now < next ) {
			logger::debug["widget"] << this -> _name << ": " << (next - now).count()
				<< "ms remaining (interval=" << this -> interval() << "ms)" << std::endl;
			return false;
		}

		logger::debug["widget"] << this -> _name << ": update due (interval="
			<< this -> interval() << "ms, overdue by " << (now - next).count() << "ms)" << std::endl;
	}

	return true;
}

void widget::add(const std::string& name, CONFIG::MAP *cfg) {

	std::string _name = common::unquoted(common::to_lower(common::trim_ws(std::as_const(name))));

	if ( _name.empty()) {

		logger::error["config"] << "syntax error, cannot create widget without name" << std::endl;
		return;
	} else if ( _name.find_first_not_of("abcdefghijklmnopqrstuvwxyz1234567890_") != std::string::npos ||
		!std::isalpha(( unsigned char )_name.front())) {

		logger::error["config"] << "illegal name '" << _name << "' for widget, names must begin " <<
			"with alphabetical character and can only contain characters " <<
			"from set 'abcdefghijklmnopqrstuvwxyz1234567890_'" << std::endl;
		return;
	} else if ( !cfg -> contains("class") && !cfg -> contains("type")) {

		logger::error["config"] << "cannot create widget '" << _name <<
			"', widget's class not found from configuration" << std::endl;
		return;
	}

	std::string _class;

	{
		const std::string class_key = cfg->contains("class") ? "class" : "type";
		if ( std::holds_alternative<std::string>((*cfg)[class_key]))
			_class = common::unquoted(common::to_lower(common::trim_ws(std::as_const(std::get<std::string>((*cfg)[class_key])))));
	}

	if ( _class.empty()) {

		logger::error["config"] << "cannot create widget '" << _name <<
			"', class is empty" << std::endl;
		return;
	}

	if ( std::find(widget::types.begin(), widget::types.end(), _class) == widget::types.end()) {

		logger::error["config"] << "failed to create widget '" << _name <<
			"', unknown widget class '" << _class << "'" << std::endl;
		return;
	}

	if ( this -> widgets.contains(_name)) {

		logger::error["config"] << "failed to create widget '" << _name <<
			"', widget with that name is already configured" << std::endl;
		return;
	}

	try {

		if ( _class == "image" )
			this -> widgets[_name] = std::make_shared<widget::IMAGE>(_name, cfg);
		else if ( _class == "ttf" )
			this -> widgets[_name] = std::make_shared<widget::TTF>(_name, cfg);
		else if ( _class == "linechart" )
			this -> widgets[_name] = std::make_shared<widget::LINECHART>(_name, cfg);
		else if ( _class == "curvechart" )
			this -> widgets[_name] = std::make_shared<widget::CURVECHART>(_name, cfg);
		else if ( _class == "bar" )
			this -> widgets[_name] = std::make_shared<widget::BAR>(_name, cfg);
		else if ( _class == "gauge" )
			this -> widgets[_name] = std::make_shared<widget::GAUGE>(_name, cfg);
		else if ( _class == "clock" )
			this -> widgets[_name] = std::make_shared<widget::CLOCK>(_name, cfg);
		else logger::error["config"] << "failed to add widget '" << _name << "', unsupported class '" << _class << "'" << std::endl;

	} catch ( const std::exception& e ) {
		logger::error["config"] << "failed to create widget '" << _name << "', reason: " << e.what() << std::endl;
		if ( this -> widgets.contains(_name))
			this -> widgets.erase(_name);
	}

}

common::lowercase_map<std::shared_ptr<widget::WIDGET>>::iterator widget::begin() {
	return this -> widgets.begin();
}

common::lowercase_map<std::shared_ptr<widget::WIDGET>>::iterator widget::end() {
	return this -> widgets.end();
}

common::lowercase_map<std::shared_ptr<widget::WIDGET>>::size_type widget::size() {
	return this -> widgets.size();
}

bool widget::contains(const std::string& name) {
	return this -> widgets.contains(name);
}

void widget::erase(const std::string& name) {

	if ( this -> widgets.contains(name))
		this -> widgets.erase(name);
}

bool widget::empty() {
	return this -> widgets.empty();
}

widget::WIDGET* widget::operator [](const std::string& name) {
	return !this -> widgets.contains(name) ? nullptr : this -> widgets[name].get();
}

widget::~widget() {
	this -> widgets.clear();
}

std::ostream& operator <<(std::ostream& os, widget::WIDGET const& w) {

	os << "widget '" << w.name() << "' {\n";
	os << common::padding(2) << "class" << common::padding(20 - 5) << "'" << w.type() << "'\n";

	for ( const auto& [k, v] : w._properties ) {

		os << common::padding(2) << k << common::padding(k.size() < 20 ? 20 - k.size() : 0) << v << "\n";
	}

	os << "}";
	return os;
}

std::ostream& operator <<(std::ostream& os, widget::WIDGET const *w) {

	os << *w;
	return os;
}

std::ostream& operator <<(std::ostream& os, widget const& w) {

	bool once = true;

	for ( const auto& [k, v] : w.widgets ) {

		os << ( once ? "" : "\n\n" ) << v;
		once = false;
	}

	return os;
}

std::ostream& operator <<(std::ostream& os, widget const *w) {

	os << *w;
	return os;
}
