#include <iostream>
#include <stdexcept>

#include "throws.hpp"
#include "logger.hpp"
#include "common.hpp"
#include "throws.hpp"
#include "display.hpp"
#include "driver.hpp"

std::vector<std::string> drv::list({ "dpf" });

int drv::DRIVER::pwidth() {
	return this -> _pwidth;
}

int drv::DRIVER::pheight() {
	return this -> _pheight;
}

RGBA drv::DRIVER::rgb(int x, int y) {
	return this -> blend(x, y);
}

static std::vector<RGBA>cc;

RGBA drv::DRIVER::blend(int x, int y) {

	// TODO: o = -1 ??? is this ok?

	int o = -1;
	RGBA ret({ .R = RGBA::BL.R, .G = RGBA::BL.G, .B = RGBA::BL.B, .A = 0x00 });

	int page_no = display -> page_number();

	if ( !display -> canvas.contains(page_no))
		throws << "fatal error: page " << page_no << " does not exist on canvas" << std::endl;

	if ( display -> canvas[page_no].empty())
		throws << "fatal error: page " << page_no << " has no layers" << std::endl;

	for ( auto& [l, v] : display -> canvas[page_no]) {

		RGBA p = display -> canvas[page_no][l][(y * display -> _width) + x];

		if ( p.A == 0xff ) {
			o = l;
			break;
		}
	}

	for ( auto& [l, v] : display -> canvas[page_no]) {

		if ( l < o )
			continue;

		RGBA p = display -> canvas[page_no][l][(y * display -> _width) + x];
		switch ( p.A ) {
			case 0: break;
			case 0xff:
				ret = RGBA({ .R = p.R, .G = p.G, .B = p.B, .A = 0xff });
				break;
			default:
				unsigned int R = ( p.R * p.A + ret.R * ( 0xff - p.A )) / 0xff;
				unsigned int G = ( p.G * p.A + ret.G * ( 0xff - p.A )) / 0xff;
				unsigned int B = ( p.B * p.A + ret.B * ( 0xff - p.A )) / 0xff;
				ret = RGBA({ .R = (unsigned char)R, .G = (unsigned char)G, .B = (unsigned char)B, .A = 0xff });
		}
	}

	return ret;
}

int drv::DRIVER::backlight() {

	return this -> _backlight;
}

// Won't work..
void drv::DRIVER::blit(const std::vector<RECT>& rects) {

	for ( const RECT& rect : rects )
		this -> blit(rect.min.x, rect.min.x, rect.max.x - rect.min.x, rect.max.y - rect.min.y);
}

void drv::DRIVER::blit() {

	this -> blit(0, 0, this -> _pwidth, this -> _pheight);
}

void drv::DRIVER::blit_fullscreen() {

	this -> blit(0, 0, this -> _pwidth, this -> _pheight);
}

void drv::DRIVER::refresh() {

	//this -> blit();
	this -> blit_fullscreen();
}

void drv::DRIVER::refresh(const std::vector<RECT>& rects) {

	this -> blit(rects);
}

void drv::DRIVER::reset_canvas() {

	this -> canvas.clear();
	for ( int y = 0; y < this -> _pheight; y++ )
		for ( int x = 0; x < this -> _pwidth; x++ )
			this -> canvas.push_back(RGBA(RGBA::TRANSPARENT.R, RGBA::TRANSPARENT.G, RGBA::TRANSPARENT.B, RGBA::TRANSPARENT.A));

}
