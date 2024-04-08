#pragma once

#include <mutex>
#include <thread>
#include "common.hpp"
#include "lowercase_map.hpp"
#include "config.hpp"
#include "expr/expression.hpp"
#include "plugin.hpp"

class plugin::UPTIME : public plugin::PLUGIN {

	public:

		virtual const std::string type() const override { return "uptime"; }

		explicit UPTIME(CONFIG::MAP *cfg);
		~UPTIME();

	static expr::VARIABLE fn_uptime(const expr::FUNCTION_ARGS& args);
	static expr::VARIABLE fn_uptime_long(const expr::FUNCTION_ARGS& args);
	static expr::VARIABLE fn_uptime_days(const expr::FUNCTION_ARGS& args);
	static expr::VARIABLE fn_uptime_hours(const expr::FUNCTION_ARGS& args);
	static expr::VARIABLE fn_uptime_minutes(const expr::FUNCTION_ARGS& args);
	static expr::VARIABLE fn_uptime_seconds(const expr::FUNCTION_ARGS& args);
	static expr::VARIABLE fn_uptime_timestamp(const expr::FUNCTION_ARGS& args);

};
