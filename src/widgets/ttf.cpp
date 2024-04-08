#include <gd.h>
#include <cstring>

#include "logger.hpp"
#include "throws.hpp"
#include "widget.hpp"
#include "driver.hpp"
#include "display.hpp"
#include "fs_funcs.hpp"
#include "rgb.hpp"
#include "expr/expression.hpp"
#include "widgets/ttf.hpp"

struct TTF_RECT {

        struct POINT {
                int x, y;
        };

        POINT lower_left, lower_right, upper_right, upper_left;

        int *gd() {
                return &this -> lower_left.x;
        }

        int width() {
                return lower_right.x - upper_left.x;
        }

        int height() {
                return lower_right.y - upper_left.y;
        }

        const std::string dump() {
                return std::to_string(lower_left.x) + ", " + std::to_string(lower_left.y) + " " +
                        std::to_string(lower_right.x) + ", " + std::to_string(lower_right.y) + " " +
                        std::to_string(upper_right.x) + ", " + std::to_string(upper_right.y) + " " +
                        std::to_string(upper_left.x) + ", " + std::to_string(upper_left.y);
        }

};

widget::TTF::TTF(const std::string& name, CONFIG::MAP *cfg) {

	this -> _name = name;

	this -> _properties = {
		{ "size", "12.0" },
		{ "color", "ffffff" },
	};

	if ( name.empty())
		throws << "failed to create ttf widget, widget is missing name" << std::endl;

	if ( cfg == nullptr || cfg -> empty())
		throws << "ttf widget " << ( name.empty() ? "" : ( name + " " )) <<
			"is missing configuration" << std::endl;

	std::vector<std::string> allowed_keys = {
		"text", "font", "size", "color", "width", "height", "align", "offset",
		"scale", "inverted", "opacity", "center", "debugborder", "debugbordercolor",
		"visible", "reload", "interval", "class"
	};

	for ( auto& [k, v] : *cfg ) {

		std::string key = k == "update" ? "interval" : k;

		if ( !std::holds_alternative<std::string>(v)) {

			logger::error["config"] << "arrays and objects are not supported by ttf widgets, ignoring '" <<
				key << "'" << std::endl;
			continue;
		}

		std::string value = std::get<std::string>(v);

		if ( !CONFIG::parse_option("ttf widget", key, value, &allowed_keys))
			continue;
		else if ( key == "class" ) continue;

		this -> _properties[key] = value;
	}

	if ( !this -> _properties.contains("text") || this -> _properties["text"].empty())
		throws << "ttf widget '" << name << "' failed to initialize, required field 'text' is missing from configuration" << std::endl;

	if ( !this -> _properties.contains("font") || this -> P2S("font").empty())
		throws << "ttf widget '" << name << "' failed to initialize, required field 'font' is missing from configuration" << std::endl;

	if ( !this -> _properties.contains("color") || this -> P2S("color").empty())
		throws << "ttf widget '" << name << "' failed to initialize, required field 'color' is missing from configuration" << std::endl;

	if ( !RGBA::check_color(this -> P2S("color")))
		throws << "ttf widget '" << name << "' failed to initalize, invalid color '" << this -> P2S("color") << "'" << std::endl;

	if ( this -> _properties.contains("interval") && this -> P2I("interval") > 0 && !this -> _properties.contains("reload"))
		this -> _properties["reload"] = "1";

	if ( this -> reloads() && !this -> _properties.contains("interval"))
		this -> _properties["reload"] = "0";
	else if ( this -> reloads() && this -> _properties.contains("interval"))
		this -> _properties["interval"] = std::to_string(this -> interval());

	this -> _needs_update = true;
}

widget::TTF::~TTF() {
	this -> _properties.clear();
}

bool widget::TTF::update() {

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

		std::string text = this -> P2S("text", "");
		std::string font = this -> P2S("font", "");

		if ( text.empty()) {

			logger::warning["widget"] << "failed to render ttf widget '" << this -> _name << "': text property is empty" << std::endl;
			this -> _needs_draw = false;

		} else if ( font.empty()) {

			logger::warning["widget"] << "failed to render ttf widget '" << this -> _name << "': font property is empty" << std::endl;
			this -> _needs_draw = false;

		} else if ( !fs::is_accessible(font)) {

			logger::warning["widget"] << "failed to render ttf widget '" << this -> _name << "': font file " << font << " does not exist or is not accesible" << std::endl;
			logger::vverbose["widget"] << "check permissions of font file " << font << "?" << std::endl;
			this -> _needs_draw = false;

		} else this -> _needs_draw = this -> render(text, font);
	}

	this -> _needs_update = false;
	this -> last_updated = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());

	return this -> _needs_draw;
}

bool widget::TTF::render(const std::string& text, const std::string& font) {

	double p_size = this -> P2N("size", 12.0);
	std::string p_color = this -> P2S("color", "ffffff");
	int p_width = this -> P2I("width", 0);
	int p_height = this -> P2I("height", 0);
	std::string p_align = this -> P2S("align", "left");
	int p_offset = this -> P2I("offset", 0);
	double p_scale = this -> P2N("scale", 1.0);
	bool p_inverted = this -> P2B("inverted", false);
	double p_opacity = this -> P2N("opacity", 1.0);
	bool p_debugborder = this -> P2B("debugborder", false);
	std::string p_debugbordercolor = this -> P2S("debugbordercolor", "ffffff");

	std::string m_text = "[Äp}§|";

	TTF_RECT b_rect;
	TTF_RECT m_rect;

	int f_color;
	int x, y;
	int t_width, t_height;

	if ( !RGBA::check_color(p_color)) {

		logger::error["widget"] << "ttf " << this -> _name << ": invalid color '" << p_color << "', setting to 'ffffff'" << std::endl;
		p_color = "ffffff";
	}

	if ( p_align != "left" && p_align != "center" && p_align != "right" ) {

		logger::error["widget"] << "ttf " << this -> _name << ": invalid alignment '" << p_align << "', available values are left, center and right, setting to left" << std::endl;
		p_align = "left";
	}

	if ( p_debugbordercolor.empty())
		p_debugbordercolor = p_color;
	else if ( !RGBA::check_color(p_debugbordercolor)) {

		if ( p_debugborder )
			logger::error["widget"] << "invalid color '" << p_debugbordercolor << "' for debugborder on ttf widget " << this -> name() << std::endl;

		p_debugbordercolor = p_color;
	}

	RGBA rgba_color(p_color);
	RGBA debug_color(p_debugbordercolor);
	std::vector<RGBA> new_bitmap;
	gdImagePtr gdImage;
	char *err = nullptr;

	if ( err = gdImageStringTTF(NULL, m_rect.gd(), 0, font.c_str(), p_size, 0., 0, 0, m_text.c_str()); err != nullptr ) {

		logger::error["widget"] << "ttf " << this -> _name << ": size calculation error: " << err << std::endl;
		return false;
	}

	if ( err = gdImageStringTTF(NULL, b_rect.gd(), 0, font.c_str(), p_size, 0., 0, 0, text.c_str()); err != nullptr ) {

		logger::error["widget"] << "ttf " << this -> _name << ": bounds size calculation error: " << err << std::endl;
		return false;
	}

	t_width = b_rect.width() + 2;
	t_height = b_rect.height() > m_rect.height() ? b_rect.height() : m_rect.height();

	gdImage = gdImageCreateTrueColor(
		p_width > 0 ? p_width : t_width,
		p_height > 0 ? p_height : t_height );
	gdImageSaveAlpha(gdImage, 1);

	if ( gdImage == nullptr ) {

		logger::error["widget"] << "ttf " << this -> _name << ": text " << this -> _name << ": CreateTrueColor failed" << std::endl;
		return false;
	}

	gdImageFill(gdImage, 0, 0, gdImageColorAllocateAlpha(gdImage, 0, 0, 0, 127));
	f_color = gdImageColorAllocateAlpha(gdImage, rgba_color.R, rgba_color.G, rgba_color.B, rgba_color.GD_alpha());

	if ( p_debugborder ) {
		int d_color = gdImageColorAllocateAlpha(gdImage, debug_color.R, debug_color.G, debug_color.B, debug_color.GD_alpha());
		gdImageRectangle(gdImage, 0, 0, t_width - 1, t_height - 1, d_color);
	}

	if ( p_width > 0 ) {

		if ( p_align == "right" )
			x = p_width - b_rect.width();
		else if ( p_align == "left" )
			x = b_rect.upper_left.x;
		else { // center
			x = ( p_width * 0.5 ) - ( b_rect.width() * 0.5 );
			if ( x < 0 )
				x = b_rect.upper_left.x;
		}

	} else x = b_rect.upper_left.x;

	y = p_height - m_rect.lower_left.y - m_rect.upper_left.y; // * 0.5;
	y += t_height * 0.12;
	x += -1 + p_offset;

	gdImageSetAntiAliased(gdImage, f_color);

	if ( err = gdImageStringTTF(gdImage, b_rect.gd(), f_color, font.c_str(), p_size, 0.0, x, y, text.c_str()); err != nullptr ) {

		logger::error["widget"] << "ttf " << this -> _name << ": text render error: " << err << std::endl;
		return false;
	}

	if (( p_scale > 0 && p_scale != 1.0 ) ||
		( p_scale < 0 && p_width > 0 )) {

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
