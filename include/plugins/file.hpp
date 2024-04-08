#pragma once

#include <mutex>
#include <thread>
#include "common.hpp"
#include "lowercase_map.hpp"
#include "config.hpp"
#include "expr/expression.hpp"
#include "plugin.hpp"

class plugin::FILE : public plugin::PLUGIN {

	public:

		virtual const std::string type() const override { return "file"; }

		explicit FILE(CONFIG::MAP *cfg);
		~FILE();

		static expr::VARIABLE fn_readline(const expr::FUNCTION_ARGS& args);
		static expr::VARIABLE fn_readconf(const expr::FUNCTION_ARGS& args);
		static expr::VARIABLE fn_exists(const expr::FUNCTION_ARGS& args);
};
