#include <cstdint>
#include <algorithm>

#include "logger.hpp"
#include "throws.hpp"
#include "plugin.hpp"
#include "mem.hpp"
#include "plugins/meminfo.hpp"

static mem_t* meminfo = nullptr;

expr::VARIABLE plugin::MEMINFO::fn_meminfo_ram_total(const expr::FUNCTION_ARGS& args) {

	mem_t::type t = mem_t::mb;

	if ( args.empty())
		logger::warning["plugin"] << "meminfo requires 1 argument, one of following strings: kb, mb, gb or percent" << std::endl;

	else {

		std::string s = args[0].string_convertible().empty() ? args[0].to_string() : "";
		s = common::to_lower(common::trim_ws(common::unquoted(common::trim_ws(s))));

		if ( s == "kb" ) t = mem_t::kb;
		else if ( s == "mb" ) t = mem_t::mb;
		else if ( s == "gb" ) t = mem_t::gb;
		else if ( s == "%" || s.starts_with("p")) t = mem_t::percent;
		else if ( s.empty()) logger::error["plugin"] << "meminfo failure, called with empty argument" << std::endl;
		else logger::error["plugin"] << "meminfo failure, argument '" << s << "' not any of kb, mb, gb or percent" << std::endl;
	}

	meminfo -> update();
	double v = meminfo -> ram.total[t];
	if ( t == mem_t::percent ) v = std::clamp(v, 0.0, 100.0);
	return v;
}

expr::VARIABLE plugin::MEMINFO::fn_meminfo_ram_used(const expr::FUNCTION_ARGS& args) {

	mem_t::type t = mem_t::mb;

	if ( args.empty())
		logger::warning["plugin"] << "meminfo requires 1 argument, one of following strings: kb, mb, gb or percent" << std::endl;

	else {

		std::string s = args[0].string_convertible().empty() ? args[0].to_string() : "";
		s = common::to_lower(common::trim_ws(common::unquoted(common::trim_ws(s))));

		if ( s == "kb" ) t = mem_t::kb;
		else if ( s == "mb" ) t = mem_t::mb;
		else if ( s == "gb" ) t = mem_t::gb;
		else if ( s == "%" || s.starts_with("p")) t = mem_t::percent;
		else if ( s.empty()) logger::error["plugin"] << "meminfo failure, called with empty argument" << std::endl;
		else logger::error["plugin"] << "meminfo failure, argument '" << s << "' not any of kb, mb, gb or percent" << std::endl;
	}

	meminfo -> update();
	double v = meminfo -> ram.used[t];
	if ( t == mem_t::percent ) v = std::clamp(v, 0.0, 100.0);
	return v;
}

expr::VARIABLE plugin::MEMINFO::fn_meminfo_ram_free(const expr::FUNCTION_ARGS& args) {

	mem_t::type t = mem_t::mb;

	if ( args.empty())
		logger::warning["plugin"] << "meminfo requires 1 argument, one of following strings: kb, mb, gb or percent" << std::endl;

	else {

		std::string s = args[0].string_convertible().empty() ? args[0].to_string() : "";
		s = common::to_lower(common::trim_ws(common::unquoted(common::trim_ws(s))));

		if ( s == "kb" ) t = mem_t::kb;
		else if ( s == "mb" ) t = mem_t::mb;
		else if ( s == "gb" ) t = mem_t::gb;
		else if ( s == "%" || s.starts_with("p")) t = mem_t::percent;
		else if ( s.empty()) logger::error["plugin"] << "meminfo failure, called with empty argument" << std::endl;
		else logger::error["plugin"] << "meminfo failure, argument '" << s << "' not any of kb, mb, gb or percent" << std::endl;
	}

	meminfo -> update();
	double v = meminfo -> ram.free[t];
	if ( t == mem_t::percent ) v = std::clamp(v, 0.0, 100.0);
	return v;
}

expr::VARIABLE plugin::MEMINFO::fn_meminfo_swap_total(const expr::FUNCTION_ARGS& args) {

	mem_t::type t = mem_t::mb;

	if ( args.empty())
		logger::warning["plugin"] << "meminfo requires 1 argument, one of following strings: kb, mb, gb or percent" << std::endl;

	else {

		std::string s = args[0].string_convertible().empty() ? args[0].to_string() : "";
		s = common::to_lower(common::trim_ws(common::unquoted(common::trim_ws(s))));

		if ( s == "kb" ) t = mem_t::kb;
		else if ( s == "mb" ) t = mem_t::mb;
		else if ( s == "gb" ) t = mem_t::gb;
		else if ( s == "%" || s.starts_with("p")) t = mem_t::percent;
		else if ( s.empty()) logger::error["plugin"] << "meminfo failure, called with empty argument" << std::endl;
		else logger::error["plugin"] << "meminfo failure, argument '" << s << "' not any of kb, mb, gb or percent" << std::endl;
	}

	meminfo -> update();
	double v = meminfo -> swap.total[t];
	if ( t == mem_t::percent ) v = std::clamp(v, 0.0, 100.0);
	return v;
}

expr::VARIABLE plugin::MEMINFO::fn_meminfo_swap_used(const expr::FUNCTION_ARGS& args) {

	mem_t::type t = mem_t::mb;

	if ( args.empty())
		logger::warning["plugin"] << "meminfo requires 1 argument, one of following strings: kb, mb, gb or percent" << std::endl;

	else {

		std::string s = args[0].string_convertible().empty() ? args[0].to_string() : "";
		s = common::to_lower(common::trim_ws(common::unquoted(common::trim_ws(s))));

		if ( s == "kb" ) t = mem_t::kb;
		else if ( s == "mb" ) t = mem_t::mb;
		else if ( s == "gb" ) t = mem_t::gb;
		else if ( s == "%" || s.starts_with("p")) t = mem_t::percent;
		else if ( s.empty()) logger::error["plugin"] << "meminfo failure, called with empty argument" << std::endl;
		else logger::error["plugin"] << "meminfo failure, argument '" << s << "' not any of kb, mb, gb or percent" << std::endl;
	}

	meminfo -> update();
	double v = meminfo -> swap.used[t];
	if ( t == mem_t::percent ) v = std::clamp(v, 0.0, 100.0);
	return v;
}

expr::VARIABLE plugin::MEMINFO::fn_meminfo_swap_free(const expr::FUNCTION_ARGS& args) {

	mem_t::type t = mem_t::mb;

	if ( args.empty())
		logger::warning["plugin"] << "meminfo requires 1 argument, one of following strings: kb, mb, gb or percent" << std::endl;

	else {

		std::string s = args[0].string_convertible().empty() ? args[0].to_string() : "";
		s = common::to_lower(common::trim_ws(common::unquoted(common::trim_ws(s))));

		if ( s == "kb" ) t = mem_t::kb;
		else if ( s == "mb" ) t = mem_t::mb;
		else if ( s == "gb" ) t = mem_t::gb;
		else if ( s == "%" || s.starts_with("p")) t = mem_t::percent;
		else if ( s.empty()) logger::error["plugin"] << "meminfo failure, called with empty argument" << std::endl;
		else logger::error["plugin"] << "meminfo failure, argument '" << s << "' not any of kb, mb, gb or percent" << std::endl;
	}

	meminfo -> update();
	double v = meminfo -> swap.free[t];
	if ( t == mem_t::percent ) v = std::clamp(v, 0.0, 100.0);
	return v;
}

plugin::MEMINFO::MEMINFO(CONFIG::MAP *cfg) {

	if ( meminfo != nullptr )
		throws << "meminfo initialization failure, already initialized" << std::endl;

	logger::vverbose["plugin"] << "initializing plugin meminfo" << std::endl;

	meminfo = new mem_t;
	CONFIG::functions.append({ "mem::total", plugin::MEMINFO::fn_meminfo_ram_total });
	CONFIG::functions.append({ "mem::used", plugin::MEMINFO::fn_meminfo_ram_used });
	CONFIG::functions.append({ "mem::free", plugin::MEMINFO::fn_meminfo_ram_free });
	CONFIG::functions.append({ "mem::swap::total", plugin::MEMINFO::fn_meminfo_swap_total });
	CONFIG::functions.append({ "mem::swap::used", plugin::MEMINFO::fn_meminfo_swap_used });
	CONFIG::functions.append({ "mem::swap::free", plugin::MEMINFO::fn_meminfo_swap_free });

}

plugin::MEMINFO::~MEMINFO() {

        if ( meminfo != nullptr ) {

                delete meminfo;
                meminfo = nullptr;
        }

        CONFIG::functions.erase("mem::total");
        CONFIG::functions.erase("mem::used");
	CONFIG::functions.erase("mem::free");
	CONFIG::functions.erase("mem::swap::total");
	CONFIG::functions.erase("mem::swap::used");
	CONFIG::functions.erase("mem::swap::free");

}
