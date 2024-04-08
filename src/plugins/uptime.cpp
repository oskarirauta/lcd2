#include "logger.hpp"
#include "throws.hpp"
#include "plugin.hpp"
#include "uptime.hpp"
#include "plugins/uptime.hpp"

static uptime_t* uptime = nullptr;

expr::VARIABLE plugin::UPTIME::fn_uptime_days(const expr::FUNCTION_ARGS& args) {

	if ( uptime == nullptr ) {

		logger::error["plugin"] << "uptime not available, plugin initialization did not succeed" << std::endl;
		return (double)0;
	}

	if ( !args.empty())
		logger::warning["plugin"] << "plugin uptime does not need any arguments" << std::endl;

	return (double)(uptime -> days());
}

expr::VARIABLE plugin::UPTIME::fn_uptime_hours(const expr::FUNCTION_ARGS& args) {

	if ( uptime == nullptr ) {

		logger::error["plugin"] << "uptime not available, plugin initialization did not succeed" << std::endl;
		return (double)0;
	}

	if ( !args.empty())
		logger::warning["plugin"] << "plugin uptime does not need any arguments" << std::endl; 

	return (double)(uptime -> hours());
}

expr::VARIABLE plugin::UPTIME::fn_uptime_minutes(const expr::FUNCTION_ARGS& args) {

	if ( uptime == nullptr ) {

		logger::error["plugin"] << "uptime not available, plugin initialization did not succeed" << std::endl;
		return (double)0;
	}

	if ( !args.empty())
		logger::warning["plugin"] << "plugin uptime does not need any arguments" << std::endl; 

	return (double)(uptime -> minutes());
}

expr::VARIABLE plugin::UPTIME::fn_uptime_seconds(const expr::FUNCTION_ARGS& args) {

	if ( uptime == nullptr ) {

		logger::error["plugin"] << "uptime not available, plugin initialization did not succeed" << std::endl;
		return (double)0;
	}

	if ( !args.empty())
		logger::warning["plugin"] << "plugin uptime does not need any arguments" << std::endl; 

	return (double)(uptime -> seconds());
}

expr::VARIABLE plugin::UPTIME::fn_uptime(const expr::FUNCTION_ARGS& args) {

	if ( uptime == nullptr ) {

		logger::error["plugin"] << "uptime not available, plugin initialization did not succeed" << std::endl;
		return "";
	}

	if ( !args.empty())
		logger::warning["plugin"] << "plugin uptime does not need any arguments" << std::endl; 

	auto data = uptime -> data();

	return ( data.days > 0 ? ( std::to_string(data.days) + "d " ) : "" ) +
		( data.hours > 0 ? ( std::to_string(data.hours) + "h " ) : "" ) +
			std::to_string(data.minutes) + "m";
}

expr::VARIABLE plugin::UPTIME::fn_uptime_long(const expr::FUNCTION_ARGS& args) {

	if ( uptime == nullptr ) {

		logger::error["plugin"] << "uptime not available, plugin initialization did not succeed" << std::endl;
		return "";
	}

	if ( !args.empty())
		logger::warning["plugin"] << "plugin uptime does not need any arguments" << std::endl; 

	auto data = uptime -> data();

	return std::to_string(data.days) + "d " +
		std::to_string(data.hours) + "h " +
		std::to_string(data.minutes) + "m " +
		std::to_string(data.seconds) + "s";
}

expr::VARIABLE plugin::UPTIME::fn_uptime_timestamp(const expr::FUNCTION_ARGS& args) {

	if ( uptime == nullptr ) {

		logger::error["plugin"] << "uptime not available, plugin initialization did not succeed" << std::endl;
		return (double)0;
	}

	if ( !args.empty())
		logger::warning["plugin"] << "plugin uptime does not need any arguments" << std::endl; 

	return (double)(uptime -> timestamp());
}

plugin::UPTIME::UPTIME(CONFIG::MAP *cfg) {

	logger::vverbose["plugin"] << "initializing plugin uptime" << std::endl;

	if ( uptime != nullptr )
		throws << "failed to initialize plugin uptime, plugin is already initialzed" << std::endl;

	try {
		uptime = new uptime_t();
	} catch ( const std::runtime_error& e ) {
		logger::error["plugin"] << "uptime plugin failed to initialize; " << e.what() << std::endl;
	}

	CONFIG::functions.append({ "uptime", plugin::UPTIME::fn_uptime });
	CONFIG::functions.append({ "uptime::long", plugin::UPTIME::fn_uptime_long });
	CONFIG::functions.append({ "uptime::days", plugin::UPTIME::fn_uptime_days });
	CONFIG::functions.append({ "uptime::hours", plugin::UPTIME::fn_uptime_hours });
	CONFIG::functions.append({ "uptime::minutes", plugin::UPTIME::fn_uptime_minutes });
	CONFIG::functions.append({ "uptime::seconds", plugin::UPTIME::fn_uptime_seconds });
	CONFIG::functions.append({ "uptime::timestamp", plugin::UPTIME::fn_uptime_timestamp });

}

plugin::UPTIME::~UPTIME() {

        CONFIG::functions.erase("uptime");
	CONFIG::functions.erase("uptime::long");
	CONFIG::functions.erase("uptime::days");
	CONFIG::functions.erase("uptime::hours");
	CONFIG::functions.erase("uptime::minutes");
	CONFIG::functions.erase("uptime::seconds");
	CONFIG::functions.erase("uptime::timestamp");

	if ( uptime != nullptr ) {

		delete uptime;
		uptime = nullptr;
	}
}
