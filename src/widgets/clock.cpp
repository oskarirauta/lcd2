#include <gd.h>
#include <cmath>
#include <ctime>
#include <algorithm>

#include "logger.hpp"
#include "throws.hpp"
#include "widget.hpp"
#include "driver.hpp"
#include "display.hpp"
#include "expr/expression.hpp"
#include "widgets/clock.hpp"

widget::CLOCK::CLOCK(const std::string& name, CONFIG::MAP *cfg) {

	this -> _name = common::unquoted(common::to_lower(common::trim_ws(std::as_const(name))));
	this -> _properties.clear();

	if ( name.empty())
		throws << "failed to create clock widget, widget is missing name" << std::endl;

	if ( cfg == nullptr || cfg -> empty())
		throws << "clock widget " << ( name.empty() ? "" : ( name + " " )) <<
			"is missing configuration" << std::endl;

	std::vector<std::string> allowed_keys = {
		"facecolor", "rimcolor", "bgcolor",
		"hourcolor", "minutecolor", "secondcolor",
		"tickcolor", "ticks", "minuteticks",
		"handwidth",
		"width", "height",
		"visible", "interval", "reload", "class", "type"
	};

	for ( auto& [k, v] : *cfg ) {

		std::string key = k == "update" ? "interval" : k;

		if ( !std::holds_alternative<std::string>(v)) {

			logger::error["config"] << "arrays and objects are not supported by clock widgets, ignoring '" <<
				key << "'" << std::endl;
			continue;
		}

		std::string value = std::get<std::string>(v);

		if ( !CONFIG::parse_option("clock widget", key, value, &allowed_keys))
			continue;
		else if ( key == "class" || key == "type" ) continue;

		this -> _properties[key] = value;
	}

	if ( !this -> _properties.contains("width") || this -> P2I("width") < 20 )
		throws << "clock widget '" << name << "' failed to initialise, missing or invalid width (minimum 20)" << std::endl;

	if ( !this -> _properties.contains("height") || this -> P2I("height") < 20 )
		throws << "clock widget '" << name << "' failed to initialise, missing or invalid height (minimum 20)" << std::endl;

	// Color defaults
	if ( !this -> _properties.contains("facecolor") || !RGBA::check_color(this -> P2S("facecolor")))
		this -> _properties["facecolor"] = "'1a1a2a'";

	if ( !this -> _properties.contains("rimcolor") || !RGBA::check_color(this -> P2S("rimcolor")))
		this -> _properties["rimcolor"] = "'666688'";

	if ( !this -> _properties.contains("hourcolor") || !RGBA::check_color(this -> P2S("hourcolor")))
		this -> _properties["hourcolor"] = "ffffff";

	if ( !this -> _properties.contains("minutecolor") || !RGBA::check_color(this -> P2S("minutecolor")))
		this -> _properties["minutecolor"] = "cccccc";

	if ( !this -> _properties.contains("secondcolor") || !RGBA::check_color(this -> P2S("secondcolor")))
		this -> _properties["secondcolor"] = "ff4444";

	if ( !this -> _properties.contains("tickcolor") || !RGBA::check_color(this -> P2S("tickcolor")))
		this -> _properties["tickcolor"] = "'888888'";

	if ( this -> _properties.contains("interval") && this -> P2I("interval") > 0 && !this -> _properties.contains("reload"))
		this -> _properties["reload"] = "1";

	if ( this -> reloads() && !this -> _properties.contains("interval"))
		this -> _properties["reload"] = "0";
	else if ( this -> reloads() && this -> _properties.contains("interval"))
		this -> _properties["interval"] = std::to_string(this -> interval());

	this -> _needs_update = true;
}

widget::CLOCK::~CLOCK() {
	this -> _properties.clear();
}

bool widget::CLOCK::update() {

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

	} else {

		this -> _needs_draw = this -> render();
	}

	this -> _needs_update = false;
	this -> last_updated = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());

	return this -> _needs_draw;
}

// Compute the endpoint of a clock hand given center, length, and angle.
// Clock angle 0 = 12 o'clock; increases clockwise.
static void hand_tip(int cx, int cy, double length, double clock_angle_deg, int *x, int *y) {

	double rad = ( clock_angle_deg - 90.0 ) * M_PI / 180.0;
	*x = cx + (int)std::round(length * std::cos(rad));
	*y = cy + (int)std::round(length * std::sin(rad));
}

bool widget::CLOCK::render() {

	int p_width    = this -> P2I("width",  100) < 20 ? 20 : this -> P2I("width",  100);
	int p_height   = this -> P2I("height", 100) < 20 ? 20 : this -> P2I("height", 100);
	int p_handwidth = std::clamp(this -> P2I("handwidth", 0), 0, 8);
	bool p_ticks        = this -> P2B("ticks",       true);
	bool p_minuteticks  = this -> P2B("minuteticks", false);

	std::string p_facecolor   = this -> P2S("facecolor",   "1a1a2a");
	std::string p_rimcolor    = this -> P2S("rimcolor",    "666688");
	std::string p_hourcolor   = this -> P2S("hourcolor",   "ffffff");
	std::string p_minutecolor = this -> P2S("minutecolor", "cccccc");
	std::string p_secondcolor = this -> P2S("secondcolor", "ff4444");
	std::string p_tickcolor   = this -> P2S("tickcolor",   "888888");

	if ( !RGBA::check_color(p_facecolor))   p_facecolor   = "1a1a2a";
	if ( !RGBA::check_color(p_rimcolor))    p_rimcolor    = "666688";
	if ( !RGBA::check_color(p_hourcolor))   p_hourcolor   = "ffffff";
	if ( !RGBA::check_color(p_minutecolor)) p_minutecolor = "cccccc";
	if ( !RGBA::check_color(p_secondcolor)) p_secondcolor = "ff4444";
	if ( !RGBA::check_color(p_tickcolor))   p_tickcolor   = "888888";

	RGBA face_color(p_facecolor);
	RGBA rim_color(p_rimcolor);
	RGBA hour_color(p_hourcolor);
	RGBA minute_color(p_minutecolor);
	RGBA second_color(p_secondcolor);
	RGBA tick_color(p_tickcolor);

	// Read system time
	std::time_t now_t = std::time(nullptr);
	std::tm *tm = std::localtime(&now_t);
	int hour   = tm -> tm_hour % 12;
	int minute = tm -> tm_min;
	int second = tm -> tm_sec;

	// Clock geometry
	int diameter = std::min(p_width, p_height);
	int cx = p_width  / 2;
	int cy = p_height / 2;
	int radius = diameter / 2;

	// Rim thickness: 2–4px depending on size
	int rim = std::max(2, diameter / 40);
	int face_diameter = diameter - rim * 2;

	// Hand lengths as fractions of radius
	double hour_len   = (radius - rim) * 0.55;
	double minute_len = (radius - rim) * 0.80;
	double second_len = (radius - rim) * 0.85;

	// Tick lengths
	int tick_outer   = radius - rim - 1;
	int hour_tick_inner   = tick_outer - std::max(4, diameter / 14);
	int minute_tick_inner = tick_outer - std::max(2, diameter / 28);

	// Hand base width (pixels thick)
	int hour_thick   = ( p_handwidth > 0 ) ? p_handwidth : std::max(2, diameter / 20);
	int minute_thick = ( p_handwidth > 0 ) ? std::max(1, p_handwidth - 1) : std::max(1, diameter / 30);

	gdImagePtr gdImage = gdImageCreateTrueColor(p_width, p_height);

	if ( gdImage == nullptr ) {
		logger::error["widget"] << "clock " << this -> _name << ": CreateTrueColor failed" << std::endl;
		return false;
	}

	gdImageSaveAlpha(gdImage, 1);

	int transparent = gdImageColorAllocateAlpha(gdImage, 0, 0, 0, 127);
	int g_face   = gdImageColorAllocateAlpha(gdImage, face_color.R,   face_color.G,   face_color.B,   face_color.GD_alpha());
	int g_rim    = gdImageColorAllocateAlpha(gdImage, rim_color.R,    rim_color.G,    rim_color.B,    rim_color.GD_alpha());
	int g_hour   = gdImageColorAllocateAlpha(gdImage, hour_color.R,   hour_color.G,   hour_color.B,   hour_color.GD_alpha());
	int g_minute = gdImageColorAllocateAlpha(gdImage, minute_color.R, minute_color.G, minute_color.B, minute_color.GD_alpha());
	int g_second = gdImageColorAllocateAlpha(gdImage, second_color.R, second_color.G, second_color.B, second_color.GD_alpha());
	int g_tick   = gdImageColorAllocateAlpha(gdImage, tick_color.R,   tick_color.G,   tick_color.B,   tick_color.GD_alpha());

	// Transparent background (corners outside the circular face)
	gdImageFill(gdImage, 0, 0, transparent);

	// Rim: outer filled circle
	gdImageFilledEllipse(gdImage, cx, cy, diameter, diameter, g_rim);

	// Face: inner filled circle
	gdImageFilledEllipse(gdImage, cx, cy, face_diameter, face_diameter, g_face);

	// Tick marks
	if ( p_ticks || p_minuteticks ) {

		for ( int i = 0; i < 60; i++ ) {

			bool is_hour_tick = ( i % 5 == 0 );
			if ( !is_hour_tick && !p_minuteticks ) continue;
			if ( is_hour_tick && !p_ticks && !p_minuteticks ) continue;

			double angle_deg = i * 6.0;
			int inner = is_hour_tick ? hour_tick_inner : minute_tick_inner;

			int ox, oy, ix, iy;
			hand_tip(cx, cy, tick_outer, angle_deg, &ox, &oy);
			hand_tip(cx, cy, inner,      angle_deg, &ix, &iy);

			gdImageSetThickness(gdImage, is_hour_tick ? 2 : 1);
			gdImageLine(gdImage, ix, iy, ox, oy, g_tick);
		}

		gdImageSetThickness(gdImage, 1);
	}

	// Hour hand
	double hour_angle   = ( hour + minute / 60.0 + second / 3600.0 ) * 30.0;
	double minute_angle = ( minute + second / 60.0 ) * 6.0;
	double second_angle = second * 6.0;

	int hx, hy, mx, my, sx, sy;
	hand_tip(cx, cy, hour_len,   hour_angle,   &hx, &hy);
	hand_tip(cx, cy, minute_len, minute_angle, &mx, &my);
	hand_tip(cx, cy, second_len, second_angle, &sx, &sy);

	// Set antialiased color and draw hands, thickest first
	gdImageSetAntiAliased(gdImage, g_hour);
	gdImageSetThickness(gdImage, hour_thick);
	gdImageLine(gdImage, cx, cy, hx, hy, gdAntiAliased);

	gdImageSetAntiAliased(gdImage, g_minute);
	gdImageSetThickness(gdImage, minute_thick);
	gdImageLine(gdImage, cx, cy, mx, my, gdAntiAliased);

	gdImageSetAntiAliased(gdImage, g_second);
	gdImageSetThickness(gdImage, 1);
	gdImageLine(gdImage, cx, cy, sx, sy, gdAntiAliased);

	gdImageSetThickness(gdImage, 1);

	// Center hub dot
	int hub = std::max(3, diameter / 20);
	gdImageFilledEllipse(gdImage, cx, cy, hub, hub, g_hour);

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
