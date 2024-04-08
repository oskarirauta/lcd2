#include <gd.h>
#include <cstring>

#include "logger.hpp"
#include "throws.hpp"
#include "widget.hpp"
#include "driver.hpp"
#include "display.hpp"
#include "fs_funcs.hpp"
#include "expr/expression.hpp"
#include "widgets/bar.hpp"

widget::BAR::BAR(const std::string& name, CONFIG::MAP *cfg) {

	this -> _name = common::unquoted(common::to_lower(common::trim_ws(std::as_const(name))));;
	this -> _properties.clear();

	if ( name.empty())
		throws << "failed to create bar widget, widget is missing name" << std::endl;

	if ( cfg == nullptr || cfg -> empty())
		throws << "bar widget " << ( name.empty() ? "" : ( name + " " )) <<
			"is missing configuration" << std::endl;

	std::vector<std::string> allowed_keys = {
		"value", "low", "high", "min", "max", "color", "colorlow", "colorhigh", "bgcolor",
		"width", "height", "smooth", "hollow", "direction", "scale", "center", "opacity",
		"inverted", "visible", "interval", "reload", "class"
	};

	for ( auto& [k, v] : *cfg ) {

		std::string key = k == "update" ? "interval" : k;

		if ( !std::holds_alternative<std::string>(v)) {

			logger::error["config"] << "arrays and objects are not supported by bar widgets, ignoring '" <<
				key << "'" << std::endl;
			continue;
		}

		std::string value = std::get<std::string>(v);

		if ( !CONFIG::parse_option("bar widget", key, value, &allowed_keys))
			continue;
		else if ( key == "class" ) continue;

		this -> _properties[key] = value;
	}

	if ( !this -> _properties.contains("min"))
		throws << "bar widget '" << name << "' failed to initalize, missing minimum value" << std::endl;

	if ( !this -> _properties.contains("max"))
		throws << "bar widget '" << name << "' failed to initalize, missing maximum value" << std::endl;

	if ( !this -> _properties.contains("low"))
		this -> _properties["low"] = -1;
	if ( !this -> _properties.contains("high"))
		this -> _properties["high"] = -1;

	if ( this -> P2I("min", -1) < 0 || this -> P2I("min", 256) > 210 ) {

		logger::error["widget"] << "invalid bar widget '" << name << "' configuration, minimum value " <<
			this -> P2I("min", -1) << " not in range 0 - 210" << std::endl;
		this -> _properties["min"] = (int)0;
	}

	if ( this -> P2I("max", -1 ) < 0 || this -> P2I("max", 300) > 250 ) {

		logger::error["widget"] << "invalid bar widget '" << name << "' configuration, maximum value " <<
			this -> P2I("max", -1) << " not in range 0 - 250" << std::endl;
		this -> _properties["min"] = (int)100;
	}

	if ( this -> P2I("max", -1) < this -> P2I("min", 0)) {

		logger::error["widget"] << "invalid bar widget '" << name << "' configuration, maximum value " <<
			this -> P2I("max", -1) << " is less than minimum " << this -> P2I("min", 0) << " value" << std::endl;
		this -> _properties["max"] = this -> P2I("min", 0) + 39;
	}

	if ( this -> _properties.contains("low") && this -> P2I("low", -1) >= 0 && (
		this -> P2I("low", -1) < this -> P2I("min", -1) || this -> P2I("low", -1) > this -> P2I("max", -1))) {

		logger::warning["widget"] << "invalid bar widget '" << name <<
			"' configuration, low value is out of minimum/maximum values" << std::endl;
		this -> _properties["low"] = -1;
	}

	if ( this -> _properties.contains("high") && this -> P2I("high", -1) >= 0 && (
		this -> P2I("high", -1) < this -> P2I("min", -1) || this -> P2I("high", -1) > this -> P2I("max", -1))) {

		logger::warning["widget"] << "invalid bar widget '" << name <<
			"' configuration, high value is out of minimum/maximum values" << std::endl;
		this -> _properties["high"] = -1;
	}

	if ( this -> _properties.contains("low") && this -> _properties.contains("high") &&
		this -> P2I("low", -1) >= 0 && this -> P2I("high", -1) >= 0 &&
		this -> P2I("high", -1 ) < this -> P2I("low", -1)) {

		logger::warning["widget"] << "invalid bar widget " << this -> _name <<
			"configuration, high is lower than low (" << this -> P2I("low", -1) << "<" <<
			this -> P2I("high", -1) <<"), disabled them" << std::endl;
		this -> _properties["low"] = -1;
		this -> _properties["high"] = -1;
	}

	if ( this -> _properties.contains("low") && this -> _properties.contains("high") &&
		this -> P2I("low", -1) >= 0 && this -> P2I("high", -1) >= 0 &&
		this -> P2I("low", -1) == this -> P2I("high", -1)) {

		logger::warning["widget"] << "invalid bar widget " << this -> _name <<
			"configuration, disabling low and high values, because they are both same(" <<
			this -> P2I("low", -1) << ")" << std::endl;
		this -> _properties["low"] = -1;
		this -> _properties["high"] = -1;
	}

	if ( !this -> _properties.contains("low"))
		this -> _properties["low"] = -1;

	if ( !this -> _properties.contains("high"))
		this -> _properties["high"] = -1;

	if ( !this -> _properties.contains("color") || this -> _properties["color"].empty() || !RGBA::check_color(this -> P2S("color"))) {

		logger::error["widget"] << "invalid bar widget '" << name << "' configuration, color missing or color is invalid" << std::endl;
		this -> _properties["fcolor"] = "'ffffff'";
	}

	if ( this -> P2I("low", -1) < 0 || !this -> _properties.contains("colorlow"))
		this -> _properties["colorlow"] = "'000000'";

	if ( this -> P2I("high", -1) < 0 || !this -> _properties.contains("colorhigh"))
		this -> _properties["colorhigh"] = "'000000'";

	if ( !this -> _properties.contains("colorlow") || this -> _properties["colorlow"].empty() || !RGBA::check_color(this -> P2S("colorlow"))) {

		logger::warning["widget"] << "invalid bar widget '" << name << "' configuration, colorlow missing or color is invalid" << std::endl;
		this -> _properties["colorlow"] = "'000000'";
	}

	if ( !this -> _properties.contains("colorhigh") || this -> _properties["colorhigh"].empty() || !RGBA::check_color(this -> P2S("colorhigh"))) {

		logger::warning["widget"] << "invalid bar widget '" << name << "' configuration, colorhigh missing or color is invalid" << std::endl;
		this -> _properties["colorhigh"] = "'000000'";
	}

	if ( !this -> _properties.contains("bgcolor") || this -> _properties["bgcolor"].empty() || !RGBA::check_color(this -> P2S("bgcolor"))) {

		logger::error["widget"] << "invalid bar widget '" << name << "' configuration, bgcolor missing or invalid" << std::endl;
		this -> _properties["bgcolor"] = "'000000'";
	}

	if ( !this -> _properties.contains("direction")) {

		logger::warning["widget"] << "invalid bar widget '" << name << "' configuration, direction missing, selecting east" << std::endl;
		this -> _properties["direction"] = "'east'";
	} else this ->_properties["direction"] = "'" + common::to_lower(common::trim_ws(std::as_const(this -> _properties["direction"]))) + "'";

	if ( !common::is_any_of(this -> P2S("direction", "east"), { "north", "east", "south", "west", "up", "right", "down", "left" })) {

		logger::warning["widget"] << "invalid bar widget '" << name << "' configuration, unsupported bar direction, selecting east" << std::endl;
		this -> _properties["direction"] = "'east'";
	} else if ( this -> P2S("direction", "east") == "up" ) this -> _properties["direction"] = "'north'";
	else if ( this -> P2S("direction", "east") == "right" ) this -> _properties["direction"] = "'east'";
	else if ( this -> P2S("direction", "east") == "down" ) this -> _properties["direction"] = "'south'";
	else if ( this -> P2S("direction", "east") == "left" ) this -> _properties["direction"] = "'west'";

	if ( !this -> _properties.contains("smooth"))
		this -> _properties["smooth"] = "0";
	else if ( this -> P2I("smooth", 0 ) >= this -> P2I("max", 0)) {

		logger::warning["widget"] << "invalid bar widget '" << name << "' smooth value, bigger than maximum, disabling smooth" << std::endl;
		this -> _properties["smooth"] = "0";

	} else if ( this -> P2I("smooth", 0) >= ( this -> P2I("max", 0) - this -> P2I("min", 0))) {

		logger::warning["widget"] << "invalid bar widget '" << name << "' smooth value, out of range between minimun and maximum" << std::endl;
		this -> _properties["smooth"] = "0";
	}

	if ( !this -> _properties.contains("width") || this -> P2I("width", -1 ) < 20 )
		throws << "bar widget '" << name << "' failed to initialize, missing or invalid width (minimum 20)" << std::endl;

	if ( !this -> _properties.contains("height") || this -> P2I("height", -1 ) < 10 )
		throws << "bar widget '" << name << "' failed to initialize, missing or invalid height (minimum 10)" << std::endl;

	//if ( !this -> _properties.contains("value") || this -> P2I("value", this -> P2I("min", -1) - 1) < 0 )
	//	throws << "bar widget '" << name << "' failed to initialize, missing or invalid value, minimum is 0" << std::endl;

	if ( this -> _properties.contains("interval") && this -> P2I("interval") > 0 && !this -> _properties.contains("reload"))
		this -> _properties["reload"] = "1";

	if ( this -> reloads() && !this -> _properties.contains("interval"))
		this -> _properties["reload"] = "0";
	else if ( this -> reloads() && this -> _properties.contains("interval"))
		this -> _properties["interval"] = std::to_string(this -> interval());

	this -> value = -1;
	this -> _needs_update = true;
}

widget::BAR::~BAR() {

	this -> _properties.clear();
}

bool widget::BAR::value_did_change() {

	int val = this -> P2I("value", this -> P2I("min", 0));
	int smoother = this -> P2I("smooth", 0);

	if ( val != this -> value && smoother > 0 ) {

		if ( val > this -> value && val > this -> value + smoother )
			val = this -> value + smoother;
		else if ( val < this -> value && val < this -> value - smoother )
			val = this -> value - smoother;
	}

	if ( val < this -> P2I("min", 0))
		val = this -> P2I("min", 0);
	else if ( val > this -> P2I("max", 0))
		val = this -> P2I("max", 0);

	bool did_change = val != this -> value;
	this -> value = val;
	return did_change;
}

bool widget::BAR::update() {

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

static unsigned char val_to_percent(unsigned char value, unsigned char min, unsigned char max) {

	int pval;
	double val = (double)value;
	double vmax = max < 1 ? (double)1 : (double)max;
	double vmin = (double)min > vmax ? (double)0 : (double)min;

	if ( val > vmax ) val = vmax;
	if ( val < vmin ) val = vmin;

	pval = (( val - vmin ) * 100 ) / ( vmax - vmin );
	if ( pval < 0 ) pval = 0;
	if ( pval > 99 ) pval = 99;

	return (unsigned char)pval;
}

bool widget::BAR::render() {

	int p_low = this -> P2I("low", -1);
	int p_high = this -> P2I("high", -1);
	int p_min = this -> P2I("min", 0);
	int p_max = this -> P2I("max", 100);
	int p_width = this -> P2I("width", 20 ) < 20 ? 20 : this -> P2I("width", 20);
	int p_height = this -> P2I("height", 10 ) < 10 ? 10: this -> P2I("height", 10);
	bool p_hollow = this -> P2B("hollow", false);
	std::string p_direction = this -> P2S("direction", "east");
	double p_scale = this -> P2N("scale", 1.0 ) < 0 ? 1.0 : this -> P2N("scale", 1.0);
	double p_opacity = this -> P2N("opacity", 1.0);
	bool p_inverted = this -> P2B("inverted", false);
	std::string p_color = this -> P2S("color", "ffffff");
	std::string p_colorlow = this -> P2S("colorlow", "000000");
	std::string p_colorhigh = this -> P2S("colorhigh", "000000");
	std::string p_bgcolor = this -> P2S("bgcolor", "444444");
	int g_fgcolor, g_bgcolor;

	if ( p_width < 1 ) {
		logger::error["widget"] << "width less than 1 for bar widget " << this -> name() << std::endl;
		p_width = 1;
	}

	if ( p_height < 1 ) {
		logger::error["widget"] << "height less than 1 for bar widget " << this -> name() << std::endl;
		p_height = 1;
	}

	if ( !RGBA::check_color(p_color)) {
		logger::error["widget"] << "invalid color '" << p_color << "' for bar widget " << this -> name() << std::endl;
		p_color = "ffffff";
	}

	if ( !RGBA::check_color(p_colorlow)) {
		logger::error["widget"] << "invalid colorlow '" << p_colorlow << "' for bar widget " << this -> name() << std::endl;
		p_colorlow = "000000";
	}

	if ( !RGBA::check_color(p_colorhigh)) {
		logger::error["widget"] << "invalid colorhigh '" << p_colorhigh << "' for bar widget " << this -> name() << std::endl;
		p_colorhigh = "000000";
	}

	if ( !RGBA::check_color(p_bgcolor)) {
		logger::error["widget"] << "invalid bgcolor '" << p_bgcolor << "' for bar widget " << this -> name() << std::endl;
		p_bgcolor = "444444";
	}

	RGBA fg_color(p_color);
	RGBA fg_colorlow(p_colorlow);
	RGBA fg_colorhigh(p_colorhigh);
	RGBA bg_color(p_bgcolor);

	std::vector<RGBA> new_bitmap;
	gdImagePtr gdImage;

	if ( gdImage = gdImageCreateTrueColor(p_width, p_height); gdImage == nullptr ) {

		logger::error["widget"] << "bar widget " << this -> _name << ", CreateTrueColor failed" << std::endl;
		return false;
	}

	gdImageSaveAlpha(gdImage, 1);

	if ( p_low > -1 && this -> value <= p_low )
		g_fgcolor = gdImageColorAllocateAlpha(gdImage, fg_colorlow.R, fg_colorlow.G, fg_colorlow.B, fg_colorlow.GD_alpha());
	else if ( p_high > -1 && this -> value >= p_high )
		g_fgcolor = gdImageColorAllocateAlpha(gdImage, fg_colorhigh.R, fg_colorhigh.G, fg_colorhigh.B, fg_colorhigh.GD_alpha());
	else
		g_fgcolor = gdImageColorAllocateAlpha(gdImage, fg_color.R, fg_color.G, fg_color.B, fg_color.GD_alpha());

	g_bgcolor = gdImageColorAllocateAlpha(gdImage, bg_color.R, bg_color.G, bg_color.B, bg_color.GD_alpha());

	gdImageFill(gdImage, 0, 0, g_bgcolor);
	gdImageSetAntiAliased(gdImage, g_fgcolor);

	unsigned char percent = val_to_percent(this -> value, p_min, p_max);
	int bar_width = (int)((double)p_width * ((double)percent * 0.01));
	int bar_height = (int)((double)p_height * ((double)percent * 0.01));

	// for hollow use gdImageRectangle
	if ( bar_height > 0 && p_direction == "north" && p_hollow )
		gdImageRectangle(gdImage, 0, p_height - 1, p_width - 1, p_height - 1 - bar_height, g_fgcolor);
	else if ( bar_height > 0 && p_direction == "north" && !p_hollow )
		gdImageFilledRectangle(gdImage, 0, p_height - 1, p_width - 1, p_height - 1 - bar_height, g_fgcolor);
	else if ( bar_width > 0 && p_direction == "east" && p_hollow )
		gdImageRectangle(gdImage, 0, 0, bar_width, p_height - 1, g_fgcolor);
	else if ( bar_height > 0 && p_direction == "south" && p_hollow )
		gdImageRectangle(gdImage, 0, 0, p_width - 1, bar_height, g_fgcolor);
	else if ( bar_height > 0 && p_direction == "south" && !p_hollow )
		gdImageFilledRectangle(gdImage, 0, 0, p_width - 1, bar_height, g_fgcolor);
	else if ( bar_width > 0 && p_direction == "west" && p_hollow )
		gdImageRectangle(gdImage, p_width - 1, 0, p_width - 1 - bar_width, p_height - 1, g_fgcolor);
	else if ( bar_width > 0 && p_direction == "west" && !p_hollow )
		gdImageFilledRectangle(gdImage, p_width - 1, 0, p_width - 1 - bar_width, p_height - 1, g_fgcolor);
	else if ( bar_width > 0 )
		gdImageFilledRectangle(gdImage, 0, 0, bar_width, p_height - 1, g_fgcolor);

	if ( p_scale > 0 && p_scale != 1.0 ) {

		int ox = gdImageSX(gdImage);
		int oy = gdImageSY(gdImage);
		int nx = ox * p_scale < 1 ? 1 : ( ox * p_scale );
		int ny = oy * p_scale < 1 ? 1 : ( oy * p_scale );

		if ( p_scale < 0 ) {  // auto-scale to widget width, but limit to widget height
			nx = p_width;
			ny = nx * oy / ox;
		}

		if ( nx < 1 ) nx = 1;
		if ( ny < 1 ) ny = 1;

		gdImagePtr scaled_image = gdImageCreateTrueColor(nx, ny);
		gdImageSaveAlpha(scaled_image, 1);
		gdImageFill(scaled_image, 0, 0, gdImageColorAllocateAlpha(scaled_image, 0, 0, 0, 127));
		gdImageCopyResized(scaled_image, gdImage, 0, 0, 0, 0, nx, ny, ox, oy);
		gdImageDestroy(gdImage);
		gdImage = scaled_image;
	}

	if ( this -> center()) {

		int ox = gdImageSX(gdImage);
		int oy = gdImageSY(gdImage);
		int cx = ( display -> width() * 0.5 ) - ( ox * 0.5 );
		int cy = 0;

		gdImagePtr center_image = gdImageCreateTrueColor(display -> width(), oy);
		gdImageSaveAlpha(center_image, 1);
		gdImageFill(center_image, 0, 0, gdImageColorAllocateAlpha(center_image, 0, 0, 0, 127));
		gdImageCopyResized(center_image, gdImage, cx, cy, 0, 0, ox, oy, ox, oy);
		gdImageDestroy(gdImage);
		gdImage = center_image;
	}

	this -> _pwidth = this -> _width;
	this -> _pheight = this -> _height;

	this -> _width = gdImage -> sx;
	this -> _height = gdImage -> sy;

	// render
	if ( this -> visible()) {

		for ( int y = 0; y < this -> _height; y++ )
			for ( int x = 0; x < this -> _width; x++ )
				new_bitmap.push_back(RGBA(gdImageGetTrueColorPixel(gdImage, x, y), p_inverted, p_opacity));

	} else new_bitmap.assign(this -> _width * this -> _height, RGBA(RGBA::TRANSPARENT));

	gdImageDestroy(gdImage);

	if ( this -> bitmap != new_bitmap ) {
		this -> bitmap = new_bitmap;
		this -> _was_visible = this -> visible();
		return true;
	}

	return false;
}
