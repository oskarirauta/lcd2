#include <iostream>
#include <stdexcept>

#include "logger.hpp"
#include "common.hpp"
#include "display.hpp"
#include "driver.hpp"

int drv::DRIVER::page() {

	if ( display.canvas.contains(display._page))
		return page();

	if ( display._page != 0 && display.canvas.contains(0)) {

		logger::warning["display"] << "canvas is missing page " << display._page << ", reverted to page 0" << std::endl;
		display._page = 0;
		return 0;

	} else if ( display._page != 0 ) {

		throw std::runtime_error("fatal error, canvas is missing page " + std::to_string(display._page) +
			" and fallback page 0");
	}

	throw std::runtime_error("fatal error, canvas is missing fallback page 0");
}

bool drv::DRIVER::set_page(int pageno) {

	if ( this -> _page == pageno && this -> canvas.contains(pageno))
		return this -> _page;

	if ( this -> canvas.contains(pageno)) {

		this -> _page = pageno;
		logger::verbose["display"] << "switch to page " << pageno << std::endl;
		return true;

	} else {

		if ( this -> _page != 0 ) {

			if ( this -> canvas.contains(this -> _page)) {

				logger::warning["display"] << "failed to switch to page " << pageno << ", remained on page " <<
					this -> _page << std::endl;
				return false;

			} else if ( this -> canvas.contains(0)) {

				logger::warning["display"] << "failed to switch to page " << pageno << ", switched to fallback page 0" << std::endl;
				this -> _page = 0;
				return true;

			}

			throw std::runtime_error("fatal error, canvas is missing page " + std::to_string(pageno) + " and fallback page 0");

		} else {

			if ( this -> canvas.contains(0)) {

				logger::warning["display"] << "failed to switch to page " << pageno << ", remained on page 0" << std::endl;
				return false;
			}

			throw std::runtime_error("fatal error, canvas is missing page " + std::to_string(pageno) + " and page 0 (fallback)");
		}
	}

	throw std::runtime_error("unknwon fatal error while trying to switch to page " + std::to_string(pageno));
}

std::vector<RGBA> drv::DRIVER::init_layer() {

	std::vector<RGBA> layer;
	for ( int i = 0; i < this -> _width * this -> _height; i++ )
		layer.push_back(RGBA(RGBA::NO));
	return layer;
}
