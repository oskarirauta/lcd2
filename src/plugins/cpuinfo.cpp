#include <cstdint>

#include "logger.hpp"
#include "throws.hpp"
#include "plugin.hpp"
#include "cpu/cpu.hpp"
#include "plugins/cpuinfo.hpp"

static cpu_t *cpu = nullptr;
static std::mutex _m;

static expr::VARIABLE fn_cpuinfo(const expr::FUNCTION_ARGS& args) {

	if ( cpu == nullptr ) {

		logger::error["plugin"] << "cannot retrieve cpu info, cpu object is not available" << std::endl;
		return "";
	}

	if ( args.empty() || !args[0].string_convertible().empty()) {

		logger::error["plugin"] << "cannot retrieve cpu info value, key not defined or is in wrong format" << std::endl;
		return "";
	}

	std::string key = common::trim_ws(common::to_lower(args[0].to_string()));
	size_t idx = -1;

	if ( key.empty()) {

		logger::error["plugin"] << "cpu::info cannot retrieve value, key is empty" << std::endl;
		return "";
	}

	if ( key != "vendor" && key != "family" && key != "model" && key != "mhz" && key != "cache" && key != "cores" &&
		key != "stepping" && key != "microcode" && key != "fpu" && key != "bogomips" && key != "cache_alignment" ) {

		logger::error["plugin"] << "invalid key " << key << " for cpu::info" << std::endl;
		logger::vverbose["plugin"] << "valid keys are: vendor, family, model, mhz, cache, cores, stepping, microcode, fpu, bogomips and cache_alignment" <<
			std::endl;
		return "";
	}

	if ( args.size() > 1 && !args[1].number_convertible().empty())
		logger::error["plugin"] << "failed to retrieve cpu info, defined cpu number is invalid" <<
			( args[1].string_convertible().empty() ? ( " " + args[1].to_string()) : "" ) << std::endl;

	else if ( args.size() > 1 ) {

		idx = (size_t)args[1].to_int();
		if ( idx < 0 || idx >= cpu -> size()) {

			logger::error["plugin"] << "cannot retrieve value for cpu" << idx << ", out of bounds, range is 0 - " << cpu -> size() << std::endl;
			idx = -1;
		}
	}

	std::string res;
	std::lock_guard<std::mutex> guard(_m);

	if ( idx == (size_t)-1 ) {

		try {
			res = cpu -> operator[](key);

		} catch ( const std::runtime_error& e ) {

			logger::error["plugin"] << "cpu::info cannot retrieve value for " << args[0].to_string() << ", reason: " << e.what() << std::endl;
			res = "";
		}

	}  else {

		try {
			res = cpu -> operator[](idx)[key];

		} catch ( const std::runtime_error& e ) {

			logger::error["plugin"] << "cpu::info cannot retrieve value for " << args[0].to_string() << ", reason: " << e.what() << std::endl;
			res = "";
		}
	}

	return res;
}

static expr::VARIABLE fn_cpuload(const expr::FUNCTION_ARGS& args) {

	if ( cpu == nullptr ) {

		logger::error["plugin"] << "cannot retrieve cpu load, cpu object is not available" << std::endl;
		return "";
	}

	std::lock_guard<std::mutex> guard(_m);

	if ( !args.empty() && args[0].number_convertible().empty()) {

		size_t i = (int)args[0].to_int();
		if ( i >= 0 && i <= cpu -> size()) {

			try {
				return cpu -> operator[](i).load();

			} catch ( const std::runtime_error& e ) {

				logger::error["plugin"] << "failed to retrieve cpu load, reason: " << e.what() << std::endl;
				return cpu -> load();
			}

		} else logger::error["plugin"] << "cannot retrieve cpu load for cpu" << args[0].to_int() << ", out of bounds, range is 0 - " << cpu -> size() << std::endl;

	} else if ( !args.empty())
		logger::warning["plugin"] << "argument " << args[0].to_string() << " is not convertible to number, cpu::load function argument must be number" << std::endl;

	return cpu -> load();
}

int plugin::CPUINFO::interval() {

	return 850;
}


bool plugin::CPUINFO::update() {

	if ( !_enabled || cpu == nullptr )
		return false;

	std::chrono::milliseconds next = this -> last_updated + std::chrono::milliseconds(this -> interval());
	std::chrono::milliseconds now = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());

	if ( now < next )
		return false;

	std::lock_guard<std::mutex> guard(_m);
	cpu -> update();

	return true;
}

plugin::CPUINFO::CPUINFO(CONFIG::MAP *cfg) {

	if ( cpu != nullptr )
		throws << "cpuinfo initialization failure, cpu object already initialized" << std::endl;

	if ( this -> _enabled ) {

		logger::vverbose["plugin"] << "preparing cpuinfo plugin" << std::endl;

		try {
			cpu = new cpu_t(4);
			this -> last_updated = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());

		} catch ( const std::runtime_error &e ) {

			logger::error["plugin"] << "plugin " << this -> type() << " failed to initialize, reason: " << e.what() << std::endl;
			this -> _enabled = false;
		}

	} else logger::debug["plugin"] << "plugin " << this -> type() << " is disabled" << std::endl;

	if ( this -> _enabled ) {

		logger::vverbose["plugin"] << "plugin " << this -> type() << " initialized" << std::endl;

		CONFIG::functions.append({ "cpu::info", fn_cpuinfo });
		CONFIG::functions.append({ "cpu::load", fn_cpuload });
	}
}

plugin::CPUINFO::~CPUINFO() {

	if ( cpu != nullptr ) {

		delete cpu;
		cpu = nullptr;
	}

	CONFIG::functions.erase("cpu::info");
	CONFIG::functions.erase("cpu::load");
}
