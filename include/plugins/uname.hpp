#pragma once

#include <mutex>
#include <thread>
#include "common.hpp"
#include "lowercase_map.hpp"
#include "config.hpp"
#include "expr/expression.hpp"
#include "plugin.hpp"

class plugin::UNAME : public plugin::PLUGIN {

	public:

		virtual const std::string type() const override { return "uname"; }

		explicit UNAME(CONFIG::MAP *cfg);
		~UNAME();

	static expr::VARIABLE fn_sysname(const expr::FUNCTION_ARGS& args);
	static expr::VARIABLE fn_nodename(const expr::FUNCTION_ARGS& args);
	static expr::VARIABLE fn_release(const expr::FUNCTION_ARGS& args);
	static expr::VARIABLE fn_version(const expr::FUNCTION_ARGS& args);
	static expr::VARIABLE fn_machine(const expr::FUNCTION_ARGS& args);
	static expr::VARIABLE fn_domainname(const expr::FUNCTION_ARGS& args);

};
