#include <gd.h>
#include <cstring>

#include "logger.hpp"
#include "throws.hpp"
#include "widget.hpp"
#include "driver.hpp"
#include "display.hpp"
#include "fs_funcs.hpp"
#include "expr/expression.hpp"
#include "widgets/linechart.hpp"

widget::LINECHART::LINECHART(const std::string& name, CONFIG::MAP *cfg) {

	this -> _name = common::unquoted(common::to_lower(common::trim_ws(std::as_const(name))));;
	this -> _properties.clear();

	if ( name.empty())
		throws << "failed to create linechart widget, widget is missing name" << std::endl;

	if ( cfg == nullptr || cfg -> empty())
		throws << "linechart widget " << ( name.empty() ? "" : ( name + " " )) <<
			"is missing configuration" << std::endl;

	std::vector<std::string> allowed_keys = {
		"value", "min", "max", "fgcolor", "fgcolor2", "bgcolor", "width", "height", "smooth", "scale"
		"center", "opacity", "inverted", "visible", "use_cycles", "interval", "reload", "class"
	};

	for ( auto& [k, v] : *cfg ) {

		std::string key = k == "update" ? "interval" : k;
		if ( common::is_any_of(key, { "in_cycles", "incycles", "usecycles", "cycles", "cycle_interval" }))
			key = "use_cycles";

		if ( !std::holds_alternative<std::string>(v)) {

			logger::error["config"] << "arrays and objects are not supported by linechart widgets, ignoring '" <<
				key << "'" << std::endl;
			continue;
		}

		std::string value = std::get<std::string>(v);

		if ( !CONFIG::parse_option("linechart widget", key, value, &allowed_keys))
			continue;
		else if ( key == "class" ) continue;

		this -> _properties[key] = value;
	}

	if ( !this -> _properties.contains("min"))
		throws << "linechart widget '" << name << "' failed to initalize, missing minimum value" << std::endl;

	if ( !this -> _properties.contains("max"))
		throws << "linechart widget '" << name << "' failed to initalize, missing maximum value" << std::endl;

	if ( this -> P2I("min", -1) < 0 || this -> P2I("min", 256) > 210 ) {

		logger::error["widget"] << "invalid linechart widget '" << name << "' configuration, minimum value " <<
			this -> P2I("min", -1) << " not in range 0 - 210" << std::endl;
		this -> _properties["min"] = (int)0;
	}

	if ( this -> P2I("max", -1 ) < 0 || this -> P2I("max", 300) > 250 ) {

		logger::error["widget"] << "invalid linechart widget '" << name << "' configuration, maximum value " <<
			this -> P2I("max", -1) << " not in range 0 - 250" << std::endl;
		this -> _properties["min"] = (int)100;
	}

	if ( this -> P2I("max", -1) < this -> P2I("min", 0)) {

		logger::error["widget"] << "invalid linechart widget '" << name << "' configuration, maximum value " <<
			this -> P2I("max", -1) << " is less than minimum " << this -> P2I("min", 0) << " value" << std::endl;
		this -> _properties["max"] = this -> P2I("min", 0) + 39;
	}

	if ( !this -> _properties.contains("fgcolor") || this -> _properties["fgcolor"].empty() || !RGBA::check_color(this -> P2S("fgcolor"))) {

		logger::error["widget"] << "invalid linechart widget '" << name << "' configuration, fgcolor missing or color is invalid" << std::endl;
		this -> _properties["fcolor"] = "'ffffff'";
	}

	if ( !this -> _properties.contains("fgcolor2") || this -> _properties["fgcolor2"].empty() || !RGBA::check_color(this -> P2S("fgcolor2"))) {

		logger::error["widget"] << "invalid linechart widget '" << name << "' configuration, fgcolor2 missing or color is invalid" << std::endl;
		this -> _properties["fcolor2"] = "'aaaaaa'";
	}

	if ( !this -> _properties.contains("bgcolor") || this -> _properties["bgcolor"].empty() || !RGBA::check_color(this -> P2S("bgcolor"))) {

		logger::error["widget"] << "invalid linechart widget '" << name << "' configuration, bgcolor missing or invalid" << std::endl;
		this -> _properties["bgcolor"] = "'444444'";
	}

	if ( int smooth = this -> P2I("smooth", 0); smooth < 0 || smooth > 85 )
		this -> _properties["smooth"] = smooth < 0 ? 0 : ( smooth > 85 ? 85 : smooth );

	if ( !this -> _properties.contains("width") || this -> P2I("width", -1 ) < 20 )
		throws << "linechart widget '" << name << "' failed to initialize, missing or invalid width (minimum 20)" << std::endl;

	if ( !this -> _properties.contains("height") || this -> P2I("height", -1 ) < 10 )
		throws << "linechart widget '" << name << "' failed to initialize, missing or invalid height (minimum 10)" << std::endl;

	//if ( !this -> _properties.contains("value") || this -> P2I("value", this -> P2I("min", -1) - 1) < 0 )
	//	throws << "linechart widget '" << name << "' failed to initialize, missing or invalid value, minimum is 0" << std::endl;

	if ( !this -> _properties.contains("use_cycles"))
		this -> _properties["use_cycles"] = "0";

	if ( this -> _properties.contains("use_cycles") && this -> P2I("use_cycles", 0) != 1 )
		this -> _properties["use_cycles"] = "0";

	(void)(this -> use_cycles());

	if ( this -> _properties.contains("interval") && this -> P2I("interval", 0) > 0 && !this -> _properties.contains("reload"))
		this -> _properties["reload"] = "1";

	if ( this -> reloads() && !this -> _properties.contains("interval"))
		this -> _properties["reload"] = "0";

	(void)(this -> interval());

	if ( this -> use_cycles())
		this -> _cycle = this -> interval();
	else this -> _cycle = -1;

	this -> next_value = this -> P2I("min", 0);
	this -> _needs_update = true;
}

widget::LINECHART::~LINECHART() {

	this -> _properties.clear();
	this -> _values.clear();
}

bool widget::LINECHART::update() {

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

		this -> next_value = this -> P2I("value", 0);

		if ( this -> next_value < this -> P2I("min"))
			this -> next_value = this -> P2I("min");
		else if ( this -> next_value > this -> P2I("max"))
			this -> next_value = this -> P2I("max");

		this -> _needs_draw = this -> render();
	}

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

bool widget::LINECHART::render() {

	int p_min = this -> P2I("min", 0);
	int p_max = this -> P2I("max", 100);
	int p_value = this -> next_value < p_min ? p_min : ( this -> next_value > p_max ? p_max : this -> next_value );
	int p_width = this -> P2I("width", 20 ) < 20 ? 20 : this -> P2I("width", 20);
	int p_height = this -> P2I("height", 10 ) < 10 ? 10: this -> P2I("height", 10);
	int p_smooth = this -> P2I("smooth", 0);
	double p_scale = this -> P2N("scale", 1.0 ) < 0 ? 1.0 : this -> P2N("scale", 1.0);
	double p_opacity = this -> P2N("opacity", 1.0);
	bool p_inverted = this -> P2B("inverted", false);
	std::string p_fgcolor = this -> P2S("fgcolor", "ffffff");
	std::string p_fgcolor2 = this -> P2S("fgcolor2", "aaaaaa");
	std::string p_bgcolor = this -> P2S("bgcolor", "444444");
	int g_fgcolor, g_fgcolor2, g_bgcolor;

	if ( p_width < 10 ) {
		logger::error["widget"] << "invalid width for linechart widget " << this -> name() << ", " << p_width << " is less than 10" << std::endl;
		p_width = 10;
	}

	if ( p_height < 10 ) {
		logger::error["widget"] << "invalid height for linechart widget " << this -> name() << ", " << p_height << " is less than 10" << std::endl;
		p_height = 10;
	}

	if ( !RGBA::check_color(p_fgcolor)) {
		logger::error["widget"] << "invalid fgcolor '" << p_fgcolor << "' for linechart widget " << this -> name() << std::endl;
		p_fgcolor = "ffffff";
	}

	if ( !RGBA::check_color(p_fgcolor2)) {
		logger::error["widget"] << "invalid fgcolor2 '" << p_fgcolor2 << "' for linechart widget " << this -> name() << std::endl;
		p_fgcolor2 = "aaaaaa";
	}

	if ( !RGBA::check_color(p_bgcolor)) {
		logger::error["widget"] << "invalid bgcolor '" << p_bgcolor << "' for linechart widget " << this -> name() << std::endl;
		p_bgcolor = "444444";
	}

	RGBA fg_color(p_fgcolor);
	RGBA fg_color2(p_fgcolor2);
	RGBA bg_color(p_bgcolor);

	// never shrink it; it might be grown back on other page..

	if ( this -> _longest_width < (size_t)p_width) {

		bool is_empty = this -> _values.empty();
		this -> _longest_width = (size_t)p_width;

		if ( !is_empty )
			std::reverse(this -> _values.begin(), this -> _values.end());

		while ( this -> _values.size() < _longest_width)
			this -> _values.push_back(0);

		if ( !is_empty )
			std::reverse(this -> _values.begin(), this -> _values.end());
	}

	int next_p = this -> smoother(
				val_to_percent(p_value, p_min, p_max),
				p_smooth );


	std::rotate(this -> _values.begin(), this -> _values.begin() + 1, this -> _values.end());
	this -> _values[this -> _longest_width - 1 ] = next_p;

	std::vector<RGBA> new_bitmap;
	gdImagePtr gdImage;

	if ( gdImage = gdImageCreateTrueColor(p_width, p_height); gdImage == nullptr ) {

		logger::error["widget"] << "linechart widget " << this -> _name << ", CreateTrueColor failed" << std::endl;
		return false;
	}

	gdImageSaveAlpha(gdImage, 1);

	g_fgcolor = gdImageColorAllocateAlpha(gdImage, fg_color.R, fg_color.G, fg_color.B, fg_color.GD_alpha());
	g_fgcolor2 = gdImageColorAllocateAlpha(gdImage, fg_color2.R, fg_color2.G, fg_color2.B, fg_color2.GD_alpha());
	g_bgcolor = gdImageColorAllocateAlpha(gdImage, bg_color.R, bg_color.G, bg_color.B, bg_color.GD_alpha());

	gdImageFill(gdImage, 0, 0, g_bgcolor);
	gdImageSetAntiAliased(gdImage, g_fgcolor);

	unsigned char prev = 250; // by design out of percentage range

	for ( int i = 0; i < p_width; i++ ) {

		unsigned char cur = this -> _values[(this -> _longest_width - p_width) + i];
		double pval1 = ( p_height - 1 ) * ( cur * 0.01 );

		int y1 = ( p_height - 1 ) - (int)pval1;
		int y2 = y1;

		if ( prev <= 100 ) {

			double pval2 = ( p_height - 1 ) * ( prev * 0.01 );
			y2 = ( p_height - 1 ) - (int)pval2;

			if ( y2 < y1 ) y2 += 1;
			else if ( y2 > y1 ) y2 -= 1;

		}

		if ( y2 == y1 ) {

			gdImageSetPixel(gdImage, i, y1, g_fgcolor);

			if ( y1 + 1 < p_height - 1 )
				gdImageLine(gdImage, i, y1 + 1, i, p_height, g_fgcolor2);
		} else {

			gdImageLine(gdImage, i, y1, i, y2, g_fgcolor);

			if ( y2 + 1 < p_height - 1 )
				gdImageLine(gdImage, i, y2 + 1, i, p_height, g_fgcolor2);
		}

		prev = cur;
	}

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

int widget::LINECHART::smoother(int value, int smooth) {

	if ( !smooth )
		return value;

	int prev_p = this -> _values.back();
	int next_p = value;
	int smoother = smooth;

	double d = (double)smoother;

	// attempt for small curve
	if ( prev_p < next_p && prev_p < 12 && next_p > 85 )
		next_p = 34;
	else if ( prev_p < next_p && prev_p < 36 && next_p > 93 )
		next_p = 82;
	else if ( prev_p > next_p && prev_p > 88 && next_p < 16 )
		next_p = 75;
	else if ( prev_p > next_p && prev_p > 74 && next_p < 13 )
		next_p = 20;

	if ( next_p > prev_p && next_p > prev_p + 93 )
		d = d * 1.25;
	else if ( next_p > prev_p && next_p > prev_p + 90 )
		d = d * 0.98;
	else if ( next_p > prev_p && next_p > prev_p + 80 )
		d = d * 0.95;
	else if ( next_p > prev_p && next_p > prev_p + 70 )
		d = d * 0.92;
	else if ( next_p > prev_p && next_p > prev_p + 60 )
		d = d * 0.88;
	else if ( next_p > prev_p && next_p > prev_p + 50 )
		d = d * 0.83;
	else if ( next_p > prev_p && next_p > prev_p + 40 )
		d = d * 0.76;
	else if ( next_p > prev_p && next_p > prev_p + 30 )
		d = d * 0.70;
	else if ( next_p > prev_p && next_p > prev_p + 20 )
		d = d * 0.64;
	else if ( next_p > prev_p && next_p > prev_p + 15 )
		d = d * 0.5;
	else if ( next_p > prev_p && next_p > prev_p + 10 )
		d = d * 0.45;
	else if ( next_p > prev_p && next_p > prev_p + 6 )
		d = d * 0.35;
	else if ( next_p > prev_p && next_p > prev_p + 3 )
		d = d * 0.3;

	if ( prev_p > 95 && next_p < 89 )
		d = d * 1.25;
	else if ( next_p < prev_p && next_p + 90 < prev_p )
		d = d * 0.98;
	else if ( next_p < prev_p && next_p + 80 < prev_p )
		d = d * 0.95;
	else if ( next_p < prev_p && next_p + 70 < prev_p )
		d = d * 0.92;
	else if ( next_p < prev_p && next_p + 60 < prev_p )
		d = d * 0.88;
	else if ( next_p < prev_p && next_p + 50 < prev_p )
		d = d * 0.83;
	else if ( next_p < prev_p && next_p + 40 < prev_p )
		d = d * 0.76;
	else if ( next_p < prev_p && next_p + 30 < prev_p )
		d = d * 0.70;
	else if ( next_p < prev_p && next_p + 20 < prev_p )
		d = d * 0.64;
	else if ( next_p < prev_p && next_p + 15 < prev_p )
		d = d * 0.5;
	else if ( next_p < prev_p && next_p + 10 < prev_p )
		d = d * 0.45;
	else if ( next_p < prev_p && next_p + 6 < prev_p )
		d = d * 0.35;
	else if ( next_p < prev_p && next_p + 3 < prev_p )
		d = d * 0.3;

	smoother = (int)d;

	if ( smoother == 0 )
		return value;

	if ( smoother > 0 && next_p != this -> _values.back()) {

		if ( prev_p < 98 && next_p > prev_p + 1 ) {

			while ( smoother > 0 && next_p > prev_p + 1 ) {
				next_p--;
				smoother--;
			}

		} else if ( prev_p > 2 && next_p < prev_p - 1 && next_p > 4 ) {

			while ( smoother > 0 && next_p < prev_p - 1 ) {
				next_p++;
				smoother--;
			}
		}
	}

	return next_p;
}
