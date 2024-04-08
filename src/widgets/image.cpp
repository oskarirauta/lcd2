#include <gd.h>
#include <cstring>

#include "logger.hpp"
#include "throws.hpp"
#include "widget.hpp"
#include "driver.hpp"
#include "display.hpp"
#include "fs_funcs.hpp"
#include "expr/expression.hpp"
#include "widgets/image.hpp"

widget::IMAGE::IMAGE(const std::string& name, CONFIG::MAP *cfg) {

	this -> _name = common::unquoted(common::to_lower(common::trim_ws(std::as_const(name))));;
	this -> _properties.clear();

	if ( name.empty())
		throws << "failed to create image widget, widget is missing name" << std::endl;

	if ( cfg == nullptr || cfg -> empty())
		throws << "image widget " << ( name.empty() ? "" : ( name + " " )) <<
			"is missing configuration" << std::endl;

	std::vector<std::string> allowed_keys = {
		"file", "width", "height", "scale", "visible", "inverted",
		"center", "opacity", "reload", "interval", "class"
	};

	for ( auto& [k, v] : *cfg ) {

		std::string key = k == "update" ? "interval" : k;

		if ( !std::holds_alternative<std::string>(v)) {

			logger::error["config"] << "arrays and objects are not supported by image widgets, ignoring '" <<
				key << "'" << std::endl;
			continue;
		}

		std::string value = std::get<std::string>(v);

		if ( !CONFIG::parse_option("image widget", key, value, &allowed_keys))
			continue;
		else if ( key == "class" ) continue;

		this -> _properties[key] = value;
	}

	if ( !this -> _properties.contains("file") || this -> P2S("file").empty())
		throws << "image widget '" << name << "' failed to initialize, required field 'file' is missing from configuration" << std::endl;

	if ( this -> _properties.contains("interval") && this -> P2I("interval") > 0 && !this -> _properties.contains("reload"))
		this -> _properties["reload"] = "1";

	if ( this -> reloads() && !this -> _properties.contains("interval"))
		this -> _properties["reload"] = "0";
	else if ( this -> reloads() && this -> _properties.contains("interval"))
		this -> _properties["interval"] = std::to_string(this -> interval());

	this -> _needs_update = true;
}

widget::IMAGE::~IMAGE() {
	this -> _properties.clear();
}

bool widget::IMAGE::update() {

	std::string filename;

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

		filename = this -> P2S("file", "");

		if ( filename.empty()) {

			logger::warning["widget"] << "failed to render image widget '" << this -> _name << "': file property is empty" << std::endl;
			this -> _needs_draw = false;

		} else if ( !fs::is_accessible(filename)) {

			logger::warning["widget"] << "failed to render image widget '" << this -> _name << "': file " <<
						filename << " does not exist or is not accessible" << std::endl;
			this -> _needs_draw = false;

		} else this -> _needs_draw = this -> render(filename);
	}

	this -> _needs_update = false;
	this -> last_updated = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());

	return this -> _needs_draw;
}

static bool open_file(FILE **fd, const std::string& filename, const std::string& name) {

	if ( *fd = fopen(filename.c_str(), "rb"); fd == nullptr ) {

		logger::error["widget"] << "Image " << name << ": fopen(" << filename << ") failed: " <<
			std::strerror(errno) << std::endl;

		return false;
	}

	return true;
}

bool widget::IMAGE::render(const std::string& filename) {

	bool p_inverted = this -> P2B("inverted", false);
	int p_width = this -> P2I("width", 0);
	int p_height = this -> P2I("height", 0);
	double p_scale = this -> P2N("scale", 1.0);
	double p_opacity = this -> P2N("opacity", 1.0);

	FILE *fd = nullptr;
	std::vector<RGBA> new_bitmap;
	gdImagePtr gdImage;

	if ( !open_file(&fd, filename, this -> _name))
		return false;

	gdImage = gdImageCreateFromPng(fd);
	fclose(fd);

	if ( gdImage == nullptr ) {

		if ( !open_file(&fd, filename, this -> _name))
			return false;

		gdImage = gdImageCreateFromJpeg(fd);
		fclose(fd);
	}

	if ( gdImage == nullptr ) {

		if ( !open_file(&fd, filename, this -> _name))
			return false;

		gdImage = gdImageCreateFromGif(fd);
		fclose(fd);
	}

	if ( gdImage == nullptr ) {

		if ( !open_file(&fd, filename, this -> _name))
			return false;

		gdImage = gdImageCreateFromBmp(fd);
		fclose(fd);
	}

	if ( gdImage == nullptr ) {

		logger::error["widget"] << "Image " << this -> _name << ": CreateFromPng/Jpeg/Gif/Bmp (" << filename <<
			") failed" << std::endl;
		return false;
	}

	if (( p_width > 0 || p_height > 0 ) && ( p_scale == 1.0 || p_scale == 0 )) {

		int ox = gdImageSX(gdImage);
		int oy = gdImageSY(gdImage);
		int nx = ox;
		int ny = oy;
		double w_fac = ((double)p_width / (double)ox);
		double h_fac = ((double)p_height / (double)oy);

		if ( w_fac == 0 ) w_fac = h_fac + 1;
		if ( h_fac == 0 ) h_fac = w_fac + 1;

		if ( w_fac > h_fac ) {
			nx = h_fac * ox;
			ny = p_height;
		} else {
			nx = p_width;
			ny = w_fac * oy;
		}

		if ( nx < 1 ) nx = 1;
		if ( ny < 1 ) ny = 1;

		gdImagePtr scaled_image = gdImageCreateTrueColor(nx, ny);
		gdImageSaveAlpha(scaled_image, true);
		gdImageFill(scaled_image, 0, 0, gdImageColorAllocateAlpha(scaled_image, 0, 0, 0, 127));
		gdImageCopyResized(scaled_image, gdImage, 0, 0, 0, 0, nx, ny, ox, oy);
		gdImageDestroy(gdImage);
		gdImage = scaled_image;
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
