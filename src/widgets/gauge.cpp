#include <gd.h>
#include <cmath>
#include <algorithm>

#include "logger.hpp"
#include "throws.hpp"
#include "widget.hpp"
#include "driver.hpp"
#include "display.hpp"
#include "expr/expression.hpp"
#include "widgets/gauge.hpp"

widget::GAUGE::GAUGE(const std::string& name, CONFIG::MAP *cfg) {

	this -> _name = common::unquoted(common::to_lower(common::trim_ws(std::as_const(name))));
	this -> _properties.clear();

	if ( name.empty())
		throws << "failed to create gauge widget, widget is missing name" << std::endl;

	if ( cfg == nullptr || cfg -> empty())
		throws << "gauge widget " << ( name.empty() ? "" : ( name + " " )) <<
			"is missing configuration" << std::endl;

	std::vector<std::string> allowed_keys = {
		"value", "min", "max",
		"fgcolor", "trackcolor", "bgcolor",
		"needlecolor", "needle",
		"low", "high", "colorlow", "colorhigh",
		"width", "height", "linewidth",
		"startangle", "sweepangle",
		"visible", "interval", "reload", "class", "type"
	};

	for ( auto& [k, v] : *cfg ) {

		std::string key = k == "update" ? "interval" : k;

		if ( !std::holds_alternative<std::string>(v)) {

			logger::error["config"] << "arrays and objects are not supported by gauge widgets, ignoring '" <<
				key << "'" << std::endl;
			continue;
		}

		std::string value = std::get<std::string>(v);

		if ( !CONFIG::parse_option("gauge widget", key, value, &allowed_keys))
			continue;
		else if ( key == "class" || key == "type" ) continue;

		this -> _properties[key] = value;
	}

	if ( !this -> _properties.contains("min"))
		this -> _properties["min"] = "0";

	if ( !this -> _properties.contains("max"))
		this -> _properties["max"] = "100";

	if ( this -> P2I("max") <= this -> P2I("min")) {

		logger::error["widget"] << "invalid gauge widget '" << name << "' configuration, max must be greater than min" << std::endl;
		this -> _properties["max"] = std::to_string(this -> P2I("min") + 100);
	}

	if ( !this -> _properties.contains("low"))
		this -> _properties["low"] = "-1";
	if ( !this -> _properties.contains("high"))
		this -> _properties["high"] = "-1";

	if ( !this -> _properties.contains("fgcolor") || this -> _properties["fgcolor"].empty() || !RGBA::check_color(this -> P2S("fgcolor"))) {

		logger::error["widget"] << "invalid gauge widget '" << name << "' configuration, fgcolor missing or invalid" << std::endl;
		this -> _properties["fgcolor"] = "'44cc44'";
	}

	if ( !this -> _properties.contains("bgcolor") || this -> _properties["bgcolor"].empty() || !RGBA::check_color(this -> P2S("bgcolor"))) {

		logger::error["widget"] << "invalid gauge widget '" << name << "' configuration, bgcolor missing or invalid" << std::endl;
		this -> _properties["bgcolor"] = "'111111'";
	}

	if ( !this -> _properties.contains("colorlow") || !RGBA::check_color(this -> P2S("colorlow")))
		this -> _properties["colorlow"] = "'000000'";

	if ( !this -> _properties.contains("colorhigh") || !RGBA::check_color(this -> P2S("colorhigh")))
		this -> _properties["colorhigh"] = "'000000'";

	if ( !this -> _properties.contains("width") || this -> P2I("width") < 20 )
		throws << "gauge widget '" << name << "' failed to initialise, missing or invalid width (minimum 20)" << std::endl;

	if ( !this -> _properties.contains("height") || this -> P2I("height") < 20 )
		throws << "gauge widget '" << name << "' failed to initialise, missing or invalid height (minimum 20)" << std::endl;

	// Default linewidth: ~1/7 of the smaller dimension, at least 4px
	if ( !this -> _properties.contains("linewidth")) {
		int lw = std::max(4, std::min(this -> P2I("width"), this -> P2I("height")) / 7);
		this -> _properties["linewidth"] = std::to_string(lw);
	}

	if ( !this -> _properties.contains("startangle"))
		this -> _properties["startangle"] = "225";

	if ( !this -> _properties.contains("sweepangle"))
		this -> _properties["sweepangle"] = "270";

	if ( int sw = this -> P2I("sweepangle", 270); sw < 10 || sw > 360 )
		this -> _properties["sweepangle"] = std::to_string(std::clamp(sw, 10, 360));

	if ( this -> _properties.contains("interval") && this -> P2I("interval") > 0 && !this -> _properties.contains("reload"))
		this -> _properties["reload"] = "1";

	if ( this -> reloads() && !this -> _properties.contains("interval"))
		this -> _properties["reload"] = "0";
	else if ( this -> reloads() && this -> _properties.contains("interval"))
		this -> _properties["interval"] = std::to_string(this -> interval());

	this -> value = this -> P2I("min", 0) - 1;
	this -> _needs_update = true;
}

widget::GAUGE::~GAUGE() {
	this -> _properties.clear();
}

// Reads and clamps the current value expression; returns true if the value changed.
bool widget::GAUGE::value_did_change() {

	int val = this -> P2I("value", this -> P2I("min", 0));
	int p_min = this -> P2I("min", 0);
	int p_max = this -> P2I("max", 100);

	if ( val < p_min ) val = p_min;
	else if ( val > p_max ) val = p_max;

	bool did_change = ( val != this -> value );
	this -> value = val;
	return did_change;
}

bool widget::GAUGE::update() {

	if ( !this -> _needs_update && this -> reloads() && this -> interval() > 0 && this -> time_to_update())
		this -> _needs_update = true;
	else if ( !this -> _needs_update )
		return false;

	if ( !this -> visible() && this -> _was_visible && !this -> bitmap.empty()) {

		std::fill(this -> bitmap.begin(), this -> bitmap.end(), RGBA(RGBA::TRANSPARENT));
		this -> _was_visible = false;
		this -> _needs_draw = true;

	} else if ( !this -> visible() && !this -> _was_visible && !this -> bitmap.empty()) {

		this -> bitmap.clear();
		this -> _width = 0;
		this -> _height = 0;
		this -> _needs_draw = false;

	} else if ( !this -> visible() && !this -> _was_visible && this -> bitmap.empty()) {

		this -> _needs_draw = false;

	} else this -> _needs_draw = this -> value_did_change() ? this -> render() : false;

	this -> _needs_update = false;
	this -> last_updated = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());

	return this -> _needs_draw;
}

bool widget::GAUGE::render() {

	int p_min        = this -> P2I("min", 0);
	int p_max        = this -> P2I("max", 100);
	int p_value      = this -> value < p_min ? p_min : ( this -> value > p_max ? p_max : this -> value );
	int p_width      = this -> P2I("width", 100) < 20 ? 20 : this -> P2I("width", 100);
	int p_height     = this -> P2I("height", 100) < 20 ? 20 : this -> P2I("height", 100);
	int p_startangle = this -> P2I("startangle", 225);
	int p_sweepangle = std::clamp(this -> P2I("sweepangle", 270), 10, 360);
	bool p_needle    = this -> P2B("needle", false);
	int p_low        = this -> P2I("low", -1);
	int p_high       = this -> P2I("high", -1);

	std::string p_fgcolor     = this -> P2S("fgcolor",     "44cc44");
	std::string p_trackcolor  = this -> P2S("trackcolor",  "");
	std::string p_bgcolor     = this -> P2S("bgcolor",     "111111");
	std::string p_needlecolor = this -> P2S("needlecolor", "");
	std::string p_colorlow    = this -> P2S("colorlow",    "000000");
	std::string p_colorhigh   = this -> P2S("colorhigh",   "000000");

	if ( !RGBA::check_color(p_fgcolor)) p_fgcolor = "44cc44";
	if ( !RGBA::check_color(p_bgcolor)) p_bgcolor = "111111";

	// Determine active arc color based on threshold
	std::string active_fg = p_fgcolor;
	if ( p_low >= 0 && p_value <= p_low && RGBA::check_color(p_colorlow) && p_colorlow != "000000" )
		active_fg = p_colorlow;
	else if ( p_high >= 0 && p_value >= p_high && RGBA::check_color(p_colorhigh) && p_colorhigh != "000000" )
		active_fg = p_colorhigh;

	RGBA fg_color(active_fg);
	RGBA bg_color(p_bgcolor);

	// trackcolor defaults to a dim blend of fgcolor and bgcolor
	int tr, tg, tb;
	if ( !p_trackcolor.empty() && RGBA::check_color(p_trackcolor)) {
		RGBA tc(p_trackcolor);
		tr = tc.R; tg = tc.G; tb = tc.B;
	} else {
		tr = fg_color.R / 4 + bg_color.R * 3 / 4;
		tg = fg_color.G / 4 + bg_color.G * 3 / 4;
		tb = fg_color.B / 4 + bg_color.B * 3 / 4;
	}

	RGBA needle_color( !p_needlecolor.empty() && RGBA::check_color(p_needlecolor) ? p_needlecolor : active_fg );

	// Ring geometry
	int diameter    = std::min(p_width, p_height);
	int cx          = p_width / 2;
	int cy          = p_height / 2;
	int radius      = diameter / 2;

	int p_linewidth = this -> P2I("linewidth", std::max(4, diameter / 7));
	p_linewidth = std::clamp(p_linewidth, 2, radius - 1);

	int inner_radius   = radius - p_linewidth;
	int inner_diameter = inner_radius * 2;

	int end_angle   = p_startangle + p_sweepangle;

	double ratio = ( p_max - p_min ) != 0 ? (double)(p_value - p_min) / (double)(p_max - p_min) : 0.0;
	int value_end_angle = p_startangle + (int)(ratio * (double)p_sweepangle);

	gdImagePtr gdImage = gdImageCreateTrueColor(p_width, p_height);

	if ( gdImage == nullptr ) {
		logger::error["widget"] << "gauge " << this -> _name << ": CreateTrueColor failed" << std::endl;
		return false;
	}

	gdImageSaveAlpha(gdImage, 1);

	int g_fg     = gdImageColorAllocateAlpha(gdImage, fg_color.R, fg_color.G, fg_color.B, fg_color.GD_alpha());
	int g_bg     = gdImageColorAllocateAlpha(gdImage, bg_color.R, bg_color.G, bg_color.B, bg_color.GD_alpha());
	int g_track  = gdImageColorAllocateAlpha(gdImage, tr, tg, tb, 0);
	int g_needle = gdImageColorAllocateAlpha(gdImage, needle_color.R, needle_color.G, needle_color.B, needle_color.GD_alpha());

	// 1. Fill entire canvas with bgcolor
	gdImageFill(gdImage, 0, 0, g_bg);

	// 2. Draw full track arc (pie from center)
	gdImageFilledArc(gdImage, cx, cy, diameter, diameter, p_startangle, end_angle, g_track, gdPie);

	// 3. Draw value arc on top of track
	if ( value_end_angle > p_startangle )
		gdImageFilledArc(gdImage, cx, cy, diameter, diameter, p_startangle, value_end_angle, g_fg, gdPie);

	// 4. Hollow out center: inner circle filled with bgcolor creates the ring
	if ( inner_diameter > 0 )
		gdImageFilledEllipse(gdImage, cx, cy, inner_diameter, inner_diameter, g_bg);

	// 5. Optional needle drawn inside the hollowed face
	if ( p_needle && inner_radius > 2 ) {

		double angle_rad = ( p_startangle + ratio * (double)p_sweepangle ) * M_PI / 180.0;
		int needle_len   = inner_radius - 2;
		int nx = cx + (int)( needle_len * std::cos(angle_rad));
		int ny = cy + (int)( needle_len * std::sin(angle_rad));

		gdImageSetThickness(gdImage, 2);
		gdImageLine(gdImage, cx, cy, nx, ny, g_needle);
		gdImageSetThickness(gdImage, 1);

		// Hub dot at center
		int hub = std::max(2, p_linewidth / 4);
		gdImageFilledEllipse(gdImage, cx, cy, hub, hub, g_needle);
	}

	this -> _pwidth  = this -> _width;
	this -> _pheight = this -> _height;
	this -> _width   = gdImage -> sx;
	this -> _height  = gdImage -> sy;

	std::vector<RGBA> new_bitmap;

	for ( int y = 0; y < this -> _height; y++ )
		for ( int x = 0; x < this -> _width; x++ )
			new_bitmap.push_back(RGBA(gdImageGetTrueColorPixel(gdImage, x, y), false, 1.0));

	gdImageDestroy(gdImage);

	if ( this -> bitmap != new_bitmap ) {
		this -> bitmap = new_bitmap;
		this -> _was_visible = this -> visible();
		return true;
	}

	return false;
}
