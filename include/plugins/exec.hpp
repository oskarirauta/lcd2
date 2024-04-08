#pragma once

#include <mutex>
#include <thread>
#include "common.hpp"
#include "lowercase_map.hpp"
#include "config.hpp"
#include "expr/expression.hpp"
#include "plugin.hpp"

class plugin::EXEC : public plugin::PLUGIN {

	public:

		virtual const std::string type() const override { return "exec"; }

		explicit EXEC(CONFIG::MAP *cfg);
		~EXEC();

		static expr::VARIABLE fn_exec(const expr::FUNCTION_ARGS& args);
};
