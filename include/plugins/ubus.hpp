#pragma once

#ifdef WITH_UBUS

#include "common.hpp"
#include "config.hpp"
#include "expr/expression.hpp"
#include "plugin.hpp"

class plugin::UBUS : public plugin::PLUGIN {

	public:

		virtual const std::string type() const override { return "ubus"; }

		explicit UBUS(CONFIG::MAP *cfg);
		~UBUS();

		static expr::VARIABLE fn_ubus(const expr::FUNCTION_ARGS& args);
		static void configure(CONFIG::MAP *cfg);
};

#endif // WITH_UBUS
