#include <gd.h>
#include <cstring>
#include <algorithm>
#include <cmath>

#include "logger.hpp"
#include "throws.hpp"
#include "widget.hpp"
#include "driver.hpp"
#include "display.hpp"
#include "fs_funcs.hpp"
#include "expr/expression.hpp"
#include "widgets/curvechart.hpp"

// Catmull-Rom spline through P1..P2, t in [0,1]. P0 and P3 are the flanking
// control points that shape the curve tangents at P1 and P2.
static double catmull_rom(double p0, double p1, double p2, double p3, double t) {
	double t2 = t * t;
	double t3 = t2 * t;
	return 0.5 * ((2.0*p1) + (-p0 + p2)*t + (2.0*p0 - 5.0*p1 + 4.0*p2 - p3)*t2 + (-p0 + 3.0*p1 - 3.0*p2 + p3)*t3);
}

static unsigned char val_to_percent(unsigned char value, unsigned char min, unsigned char max) {

	int pval;
	double val  = (double)value;
	double vmax = max < 1 ? 1.0 : (double)max;
	double vmin = (double)min > vmax ? 0.0 : (double)min;

	if ( val > vmax ) val = vmax;
	if ( val < vmin ) val = vmin;

	pval = (int)((( val - vmin ) * 100 ) / ( vmax - vmin ));
	if ( pval < 0 )  pval = 0;
	if ( pval > 99 ) pval = 99;

	return (unsigned char)pval;
}

widget::CURVECHART::CURVECHART(const std::string& name, CONFIG::MAP *cfg) {

	this -> _name = common::unquoted(common::to_lower(common::trim_ws(std::as_const(name))));
	this -> _properties.clear();

	if ( name.empty())
		throws << "failed to create curvechart widget, widget is missing name" << std::endl;

	if ( cfg == nullptr || cfg -> empty())
		throws << "curvechart widget " << ( name.empty() ? "" : ( name + " " )) <<
			"is missing configuration" << std::endl;

	std::vector<std::string> allowed_keys = {
		"value", "min", "max",
		"fgcolor", "fgcolor2", "bgcolor",
		"gridlines", "gridcolor",
		"width", "height",
		"fill", "linewidth", "samples", "smooth",
		"scale", "center", "opacity", "inverted", "visible",
		"use_cycles", "interval", "reload", "class", "type"
	};

	for ( auto& [k, v] : *cfg ) {

		std::string key = k == "update" ? "interval" : k;
		if ( common::is_any_of(key, { "in_cycles", "incycles", "usecycles", "cycles", "cycle_interval" }))
			key = "use_cycles";

		if ( !std::holds_alternative<std::string>(v)) {

			logger::error["config"] << "arrays and objects are not supported by curvechart widgets, ignoring '" <<
				key << "'" << std::endl;
			continue;
		}

		std::string value = std::get<std::string>(v);

		if ( !CONFIG::parse_option("curvechart widget", key, value, &allowed_keys))
			continue;
		else if ( key == "class" || key == "type" ) continue;

		this -> _properties[key] = value;
	}

	if ( !this -> _properties.contains("min"))
		throws << "curvechart widget '" << name << "' failed to initialise, missing minimum value" << std::endl;

	if ( !this -> _properties.contains("max"))
		throws << "curvechart widget '" << name << "' failed to initialise, missing maximum value" << std::endl;

	if ( this -> P2I("min", -1) < 0 || this -> P2I("min", 256) > 210 ) {

		logger::error["widget"] << "invalid curvechart widget '" << name << "' configuration, minimum value " <<
			this -> P2I("min", -1) << " not in range 0-210" << std::endl;
		this -> _properties["min"] = (int)0;
	}

	if ( this -> P2I("max", -1) < 0 || this -> P2I("max", 300) > 250 ) {

		logger::error["widget"] << "invalid curvechart widget '" << name << "' configuration, maximum value " <<
			this -> P2I("max", -1) << " not in range 0-250" << std::endl;
		this -> _properties["max"] = (int)100;
	}

	if ( this -> P2I("max", -1) < this -> P2I("min", 0)) {

		logger::error["widget"] << "invalid curvechart widget '" << name << "' configuration, maximum value " <<
			this -> P2I("max", -1) << " is less than minimum " << this -> P2I("min", 0) << std::endl;
		this -> _properties["max"] = this -> P2I("min", 0) + 39;
	}

	if ( !this -> _properties.contains("fgcolor") || this -> _properties["fgcolor"].empty() || !RGBA::check_color(this -> P2S("fgcolor"))) {

		logger::error["widget"] << "invalid curvechart widget '" << name << "' configuration, fgcolor missing or invalid" << std::endl;
		this -> _properties["fgcolor"] = "'44cc44'";
	}

	if ( !this -> _properties.contains("bgcolor") || this -> _properties["bgcolor"].empty() || !RGBA::check_color(this -> P2S("bgcolor"))) {

		logger::error["widget"] << "invalid curvechart widget '" << name << "' configuration, bgcolor missing or invalid" << std::endl;
		this -> _properties["bgcolor"] = "'1a1a1a'";
	}

	if ( !this -> _properties.contains("width") || this -> P2I("width", -1) < 10 )
		throws << "curvechart widget '" << name << "' failed to initialise, missing or invalid width (minimum 10)" << std::endl;

	if ( !this -> _properties.contains("height") || this -> P2I("height", -1) < 4 )
		throws << "curvechart widget '" << name << "' failed to initialise, missing or invalid height (minimum 4)" << std::endl;

	if ( int lw = this -> P2I("linewidth", 1); lw < 1 || lw > 3 )
		this -> _properties["linewidth"] = std::to_string(std::clamp(lw, 1, 3));

	if ( int sm = this -> P2I("smooth", 0); sm < 0 || sm > 85 )
		this -> _properties["smooth"] = std::to_string(std::clamp(sm, 0, 85));

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

	// Determine ring-buffer size:
	// 'samples' sets how many data points are stored.
	// Defaults to 'width' (one sample per pixel), which gives the same
	// data density as linechart but drawn as a curve.
	// Setting samples < width (e.g. samples 40, width 400) produces a
	// visibly smooth Catmull-Rom wave spanning the full chart width.
	int p_width   = this -> P2I("width", 20);
	int p_samples = this -> P2I("samples", 0);

	if ( p_samples < 4 || p_samples > 4096 )
		p_samples = p_width;

	if ( p_samples > p_width )
		p_samples = p_width;

	this -> _num_samples = (size_t)p_samples;
	this -> _values.assign(this -> _num_samples, 0);
	this -> next_value = this -> P2I("min", 0);
	this -> _needs_update = true;
}

widget::CURVECHART::~CURVECHART() {

	this -> _properties.clear();
	this -> _values.clear();
}

bool widget::CURVECHART::update() {

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

bool widget::CURVECHART::render() {

	int p_min      = this -> P2I("min", 0);
	int p_max      = this -> P2I("max", 100);
	int p_value    = this -> next_value < p_min ? p_min : ( this -> next_value > p_max ? p_max : this -> next_value );
	int p_width    = this -> P2I("width",  10) < 10 ? 10 : this -> P2I("width",  10);
	int p_height   = this -> P2I("height",  4) <  4 ?  4 : this -> P2I("height",  4);
	int p_smooth   = this -> P2I("smooth", 0);
	bool p_fill    = this -> P2B("fill", true);
	int p_linewidth = std::clamp(this -> P2I("linewidth", 1), 1, 3);
	double p_scale   = this -> P2N("scale",   1.0) < 0 ? 1.0 : this -> P2N("scale",   1.0);
	double p_opacity = this -> P2N("opacity", 1.0);
	bool p_inverted  = this -> P2B("inverted", false);
	std::string p_fgcolor  = this -> P2S("fgcolor",   "44cc44");
	std::string p_fgcolor2 = this -> P2S("fgcolor2",  "");
	std::string p_bgcolor  = this -> P2S("bgcolor",   "1a1a1a");
	int p_gridlines = std::max(0, this -> P2I("gridlines", 0));
	std::string p_gridcolor = this -> P2S("gridcolor", "");

	if ( !RGBA::check_color(p_fgcolor)) {
		logger::error["widget"] << "curvechart " << this -> _name << ": invalid fgcolor '" << p_fgcolor << "', resetting" << std::endl;
		p_fgcolor = "44cc44";
	}

	if ( p_fgcolor2.empty() || !RGBA::check_color(p_fgcolor2))
		p_fgcolor2 = p_fgcolor; // fill uses same color at reduced opacity when fgcolor2 is unset

	if ( !RGBA::check_color(p_bgcolor)) {
		logger::error["widget"] << "curvechart " << this -> _name << ": invalid bgcolor '" << p_bgcolor << "', resetting" << std::endl;
		p_bgcolor = "1a1a1a";
	}

	RGBA fg_color(p_fgcolor);
	RGBA fg_color2(p_fgcolor2);
	RGBA bg_color(p_bgcolor);

	int num_samples = (int)this -> _num_samples;

	// Apply incoming value to ring buffer
	int next_pct = this -> smoother(val_to_percent((unsigned char)p_value, (unsigned char)p_min, (unsigned char)p_max), p_smooth);

	std::rotate(this -> _values.begin(), this -> _values.begin() + 1, this -> _values.end());
	this -> _values[num_samples - 1] = (unsigned char)next_pct;

	// Create GD image
	std::vector<RGBA> new_bitmap;
	gdImagePtr gdImage;

	if ( gdImage = gdImageCreateTrueColor(p_width, p_height); gdImage == nullptr ) {

		logger::error["widget"] << "curvechart " << this -> _name << ": CreateTrueColor failed" << std::endl;
		return false;
	}

	gdImageSaveAlpha(gdImage, 1);

	int g_fgcolor  = gdImageColorAllocateAlpha(gdImage, fg_color.R,  fg_color.G,  fg_color.B,  fg_color.GD_alpha());
	int g_fgcolor2 = gdImageColorAllocateAlpha(gdImage, fg_color2.R, fg_color2.G, fg_color2.B, std::min(gdAlphaMax - 1, fg_color2.GD_alpha() + 80));
	int g_bgcolor  = gdImageColorAllocateAlpha(gdImage, bg_color.R,  bg_color.G,  bg_color.B,  bg_color.GD_alpha());

	gdImageFill(gdImage, 0, 0, g_bgcolor);

	// Horizontal gridlines at equal value intervals, drawn before chart data
	if ( p_gridlines > 1 ) {

		int g_grid;
		if ( !p_gridcolor.empty() && RGBA::check_color(p_gridcolor)) {
			RGBA gc(p_gridcolor);
			g_grid = gdImageColorAllocateAlpha(gdImage, gc.R, gc.G, gc.B, gc.GD_alpha());
		} else {
			// Default: dim blend of fgcolor and bgcolor
			g_grid = gdImageColorAllocateAlpha(gdImage,
				fg_color.R / 4 + bg_color.R * 3 / 4,
				fg_color.G / 4 + bg_color.G * 3 / 4,
				fg_color.B / 4 + bg_color.B * 3 / 4, 0);
		}

		for ( int i = 1; i < p_gridlines; i++ ) {
			int gy = (int)((double)p_height * (1.0 - (double)i / (double)p_gridlines));
			gdImageLine(gdImage, 0, gy, p_width - 1, gy, g_grid);
		}
	}

	// Compute the curve y-coordinate for every pixel column using Catmull-Rom.
	// When num_samples == p_width the sample-to-pixel mapping is 1:1 and
	// Catmull-Rom reduces to the raw data values (t==0 always).
	// When num_samples < p_width each sample spans multiple pixels and CR
	// produces a smooth interpolated curve between them.
	std::vector<int> y_curve(p_width);

	for ( int x = 0; x < p_width; x++ ) {

		double fpos = ( num_samples > 1 && p_width > 1 )
			? (double)x * ( num_samples - 1.0 ) / ( p_width - 1.0 )
			: 0.0;

		int idx = (int)fpos;
		double t = fpos - idx;

		int i0 = std::max(idx - 1, 0);
		int i1 = std::min(idx,     num_samples - 1);
		int i2 = std::min(idx + 1, num_samples - 1);
		int i3 = std::min(idx + 2, num_samples - 1);

		double cr = catmull_rom(
			(double)this -> _values[i0],
			(double)this -> _values[i1],
			(double)this -> _values[i2],
			(double)this -> _values[i3],
			t
		);

		cr = std::clamp(cr, 0.0, 99.0);
		y_curve[x] = ( p_height - 1 ) - (int)std::round(( p_height - 1 ) * ( cr * 0.01 ));
	}

	// Draw fill area below the curve (vertical lines from curve point to bottom)
	if ( p_fill ) {

		for ( int x = 0; x < p_width; x++ ) {

			int y = y_curve[x];

			if ( y < p_height - 1 )
				gdImageLine(gdImage, x, y + 1, x, p_height - 1, g_fgcolor2);
		}
	}

	// Draw the curve line itself
	gdImageSetAntiAliased(gdImage, g_fgcolor);

	if ( p_linewidth > 1 )
		gdImageSetThickness(gdImage, p_linewidth);

	int line_color = ( p_linewidth == 1 ) ? gdAntiAliased : g_fgcolor;

	for ( int x = 1; x < p_width; x++ )
		gdImageLine(gdImage, x - 1, y_curve[x - 1], x, y_curve[x], line_color);

	if ( p_linewidth > 1 )
		gdImageSetThickness(gdImage, 1);

	// Optional scale
	if ( p_scale > 0 && p_scale != 1.0 ) {

		int ox = gdImageSX(gdImage);
		int oy = gdImageSY(gdImage);
		int nx = ox * p_scale < 1 ? 1 : (int)( ox * p_scale );
		int ny = oy * p_scale < 1 ? 1 : (int)( oy * p_scale );

		if ( nx < 1 ) nx = 1;
		if ( ny < 1 ) ny = 1;

		gdImagePtr scaled = gdImageCreateTrueColor(nx, ny);
		gdImageSaveAlpha(scaled, 1);
		gdImageFill(scaled, 0, 0, gdImageColorAllocateAlpha(scaled, 0, 0, 0, 127));
		gdImageCopyResized(scaled, gdImage, 0, 0, 0, 0, nx, ny, ox, oy);
		gdImageDestroy(gdImage);
		gdImage = scaled;
	}

	// Optional horizontal center
	if ( this -> center()) {

		int ox = gdImageSX(gdImage);
		int oy = gdImageSY(gdImage);
		int cx = ( display -> width() * 0.5 ) - ( ox * 0.5 );

		gdImagePtr centered = gdImageCreateTrueColor(display -> width(), oy);
		gdImageSaveAlpha(centered, 1);
		gdImageFill(centered, 0, 0, gdImageColorAllocateAlpha(centered, 0, 0, 0, 127));
		gdImageCopyResized(centered, gdImage, cx, 0, 0, 0, ox, oy, ox, oy);
		gdImageDestroy(gdImage);
		gdImage = centered;
	}

	this -> _pwidth  = this -> _width;
	this -> _pheight = this -> _height;
	this -> _width   = gdImage -> sx;
	this -> _height  = gdImage -> sy;

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

// Exponential moving average smoother.
// smooth=0 means no smoothing; smooth=85 applies heavy smoothing.
int widget::CURVECHART::smoother(int value, int smooth) {

	if ( !smooth || this -> _values.empty())
		return value;

	double alpha = 1.0 - std::clamp(smooth, 0, 85) / 100.0;
	double prev  = (double)this -> _values.back();

	return (int)std::round(alpha * (double)value + ( 1.0 - alpha ) * prev);
}
