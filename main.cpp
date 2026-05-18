#include <iostream>

#include "logger.hpp"
#include "signal.hpp"
#include "config.hpp"
#include "display.hpp"
#include "usage.hpp"

#define LCD2_VERSION "1.0.1"

static void die_handler(int signum) {
	logger::info << SIG::to_string(signum) << " received, exiting" << std::endl;
	if ( display != nullptr )
		display->scheduler->exit_loop(true);
}

int main(int argc, char **argv) {

	usage_t usage = {
		.args = { argc, argv },
		.info = {
			.name          = "lcd2",
			.version       = LCD2_VERSION,
			.author        = "Oskari Rauta",
			.copyright     = "2024-2026, Oskari Rauta",
			.usage         = "[options]",
			.options_title = "\nOptions:",
		},
		.options = {
			{ "help",     { .key = "h", .word = "help",     .desc = "show this help and exit" }},
			{ "version",  { .key = "V", .word = "version",  .desc = "show version and exit" }},
			{ "config",   { .key = "c", .word = "config",   .desc = "configuration file",
			                .flag = usage_t::REQUIRED, .name = "file" }},
			{ "file",     { .key = "f",                     .desc = "configuration file (alias for -c)",
			                .flag = usage_t::REQUIRED, .name = "file" }},
			{ "verbose",  {             .word = "verbose",  .desc = "verbose log level" }},
			{ "vverbose", {             .word = "vverbose", .desc = "extra verbose log level" }},
			{ "debug",    { .key = "d", .word = "debug",    .desc = "debug log level (very noisy)" }},
			{ "silent",   { .key = "s", .word = "silent",   .desc = "suppress startup banner" }},
			{ "quiet",    { .key = "q", .word = "quiet",    .desc = "suppress logging to errors only" }},
		}
	};

	if ( usage["help"] || ( !usage["config"] && !usage["file"] )) {
		std::cout << usage << "\n" << usage.help() << "\n" << std::endl;
		if ( !usage["help"] )
			std::cout << "error: configuration file not set\n" << std::endl;
		return 0;
	}

	if ( usage["version"] ) {
		std::cout << usage.version() << std::endl;
		return 0;
	}

	if ( usage["debug"] )
		logger::loglevel(logger::debug);
	else if ( usage["vverbose"] )
		logger::loglevel(logger::vverbose);
	else if ( usage["verbose"] )
		logger::loglevel(logger::verbose);
	else if ( usage["quiet"] )
		logger::loglevel(logger::error);
	else
		logger::loglevel(logger::info);

	if ( !usage["silent"] || usage["quiet"] )
		std::cout << usage << "\n" << std::endl;

	std::string config_file = "lcd2.conf";
	if ( usage["config"] )
		config_file = usage["config"].value;
	else if ( usage["file"] )
		config_file = usage["file"].value;

	SIG handler = {
		.TERM = die_handler,
		.HUP  = die_handler,
		.INT  = die_handler,
		.QUIT = die_handler,
	};
	handler.install();

	CONFIG *cfg = nullptr;

	try {
		cfg = new CONFIG(config_file);
	} catch ( const std::exception& e ) {
		logger::error["config"] << e.what() << std::endl;
		if ( cfg != nullptr ) delete cfg;
		return 1;
	}

	try {
		display = new DISPLAY(cfg);
	} catch ( const std::exception& e ) {
		logger::error["display"] << e.what() << std::endl;
		delete cfg;
		return 1;
	}

	delete cfg;

	display->run();

	delete display;
	return 0;
}
