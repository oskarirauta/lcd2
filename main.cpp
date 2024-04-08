#include <iostream>

#include <gd.h>

#include "logger.hpp"
#include "signal.hpp"
#include "rgb.hpp"
#include "config.hpp"
#include "driver_classes.hpp"
#include "plugin_classes.hpp"
#include "widget_classes.hpp"
#include "display.hpp"
#include "layout.hpp"
#include "scheduler.hpp"

#include <chrono>
#include <thread>

#include "common.hpp"
#include "lowercase_map.hpp"
#include "expr/property.hpp"


#include <fstream>

static void die_handler(int signum) {

	logger::info << Signal::string(signum) << " signal received" << std::endl;
	display -> scheduler -> exit_loop(true);
}


int main(int argc, char **argv) {
	std::cout << "lcd2 project\nAuthor: Oskari Rauta <oskari.rauta@gmail.com>\n" << std::endl;

	logger::loglevel(logger::vverbose);
	//logger::loglevel(logger::info);

	std::string config_file = "./lcd4linux.conf";

	if ( argc > 1 ) {
		config_file = std::string(argv[1]);
		logger::loglevel(logger::info);
		logger::silence = true;
	}

	Signal::register_handler(die_handler, {
		{ static_cast<Signal::type>(SIGTERM), true },
		{ static_cast<Signal::type>(SIGHUP), true },
		{ static_cast<Signal::type>(SIGINT), true },
		{ static_cast<Signal::type>(SIGQUIT), true }
	});

	CONFIG *cfg = nullptr;

	try {
		cfg = new CONFIG(config_file);

	} catch ( const std::exception& e ) {

		logger::error["config"] << e.what() << std::endl;
		std::cout << "exiting" << std::endl;

		if ( cfg != nullptr )
			delete cfg;

		return -1;
	}

/*
	std::cout << "lcd4linux.conf:" << std::endl;
	std::cout << cfg << "\n" << std::endl;
*/

	try {
		display = new DISPLAY(cfg);
	} catch ( const std::exception& e ) {
		logger::error["config"] << e.what() << std::endl;
		std::cout << "exiting" << std::endl;
		delete display;
		delete cfg;
		return -1;
	}

	delete cfg;

/*
	std::cout << "\nconfig file:\n" << display << std::endl;

	std::cout << "\nlayout:\n" << display -> layout -> dump() << std::endl;
*/
	//std::cout << "\n" << display -> widgets << std::endl;

	//display -> driver -> clear();
	display -> run();

	delete display;

	return 0;

	//std::cout << "config ./lcd4linux.conf loaded" << std::endl;

	//variables -> dump();
	/*
	std::cout << "variables:" << std::endl;
	for ( auto &s : config::variables )
		std::cout << s.first << " = " << describe(s.second) << std::endl;
	*/

	std::cout << "\nDisplay:\n" << display << std::endl;;
	std::cout << "\nLayout:" << ( display -> layout != nullptr ? ( "\n" + display -> layout -> dump()) : "uninitialized" ) << std::endl;


	(*display -> widgets)["logo"] -> update();
	(*display -> widgets)["logo2"] -> update();

/*
	std::chrono::milliseconds last_updated = std::chrono::milliseconds(0);
	std::chrono::milliseconds interval = std::chrono::milliseconds(1000);
	std::chrono::milliseconds now = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());
	std::chrono::milliseconds next = now + interval;
	std::chrono::milliseconds diff = next - now;

	std::cout << "\nlast_updated: " << last_updated << " count: " << now.count() << std::endl;
	std::cout << "interval: " << interval << " count: " << interval.count() << std::endl;
	std::cout << "now: " << now << " count: " << now.count() << std::endl;
	std::cout << "next update: " << next << " count: " << next.count() << " difference: " << diff << std::endl;
*/

//	std::cout << "\nexiting" << std::endl;
//	return 1;

	std::cout << "clearing display" << std::endl;

	//std::cout << "driver: " << display -> name() << std::endl;
	//std::cout << "port: " << display -> port() << std::endl;

	display -> clear();

	std::cout << "render begins" << std::endl;

	display -> run();

	delete display;
	return 0;

	//std::cout << "wait begins" << std::endl;
	//usleep(1000);

//	return 0;

	bool needs_draw = true;

	for ( int i = 0; i < 15; i++ ) {

		std::cout << "round #" << (i + 1);

		expr::expression e("logo_seq = logo_seq < 62 ? ( logo_seq + 1 ) : logo_seq");
		expr::RESULT result;

		try {
			result = e.evaluate(&CONFIG::functions, &CONFIG::variables);
			std::cout << ", result: " << result;

		} catch ( std::runtime_error &e ) {
			result = nullptr;
			std::cout << "runtime error, " << e.what() << std::endl;
		}

		for ( const auto& [k, v] : (*display -> widgets))
			if ( (*display -> widgets)[k] -> update())
				needs_draw = true;

		if ( needs_draw ) {
			display -> layout -> render();
			usleep(10);
			display -> refresh();
			needs_draw = false;
			std::cout << " - refreshing" << std::endl;
		} else std::cout << " - no updates" << std::endl;

		usleep(1500 * 400);
	}

	usleep(1500 * 1500);

	return 1;

	int cx = 30;
	int cy = 30;

	for ( int y = 0; y < (*display -> widgets)["logo"] -> height(); y++ )
		for ( int x = 0; x < (*display -> widgets)["logo"] -> width(); x++ )
			display -> add_pixel(cx + x, cy + y, 0, 4,
				(*display -> widgets)["logo"] -> bitmap[(y * (*display -> widgets)["logo"] -> width()) + x]);

	display -> refresh();
	usleep(1000 * 1500 * 1);

	return 1;

	for ( int y = 20; y < 35; y++ )
		for ( int x = 0; x < display -> width(); x++ )
			display -> add_pixel(x, y, 0, 0,
				x > ( display -> width() * 0.5 ) ? RGBA({ .R = 0xff, .G = 0x0, .B = 0, .A = 0xff }) :
					RGBA({ .R = 0x00, .G = 0xff, .B = 0, .A = 0xff}));
			//display -> set_pixel(x, y, RGBA({ .R = 0, .G = 0xff, .B = 0, .A = 0xff }));

	//std::cout << "wait 2.." << std::endl;
	display -> refresh();
	usleep(1000 * 1200 * 1);
	//d ->

	return 1;

	display -> clear();


	



	gdImage gd_img;
	gdImagePtr img = &gd_img;
	FILE *fd;

	fd = fopen("/root/ux/static/openwrt_logo_small.png", "rb");
	img = gdImageCreateFromPng(fd);
	fclose(fd);

	for ( int x = 0; x < img->sx; x++ ) {
		for ( int y = 0; y < img->sy; y++ ) {

			int p = gdImageGetTrueColorPixel(img, x, y);
			int a = gdTrueColorGetAlpha(p);
			//int i = y * Image->width + x;

			RGBA c = RGBA({ .R = gdTrueColorGetRed(p), .G = gdTrueColorGetGreen(p), .B = gdTrueColorGetBlue(p), .A = 0xff });
			c.A = ( a == 127 ) ? 0 : 255 - a;
			display -> add_pixel(cx + x, cy + y, 0, 0, c);
		}
	}

	display -> refresh();

	usleep(1000 * 1000 * 3);

	std::cout << "wait ends" << std::endl;


	std::cout << "program exiting" << std::endl;

	return 0;
}
