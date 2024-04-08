#pragma once

#include <mutex>
#include <thread>
#include "common.hpp"
#include "lowercase_map.hpp"
#include "config.hpp"
#include "expr/expression.hpp"
#include "plugin.hpp"

class plugin::FS : public plugin::PLUGIN {

	public:

		virtual const std::string type() const override { return "fs"; }

		explicit FS(CONFIG::MAP *cfg);
		~FS();

	static expr::VARIABLE fn_capacity(const expr::FUNCTION_ARGS& args);
	static expr::VARIABLE fn_used(const expr::FUNCTION_ARGS& args);
	static expr::VARIABLE fn_free(const expr::FUNCTION_ARGS& args);
	static expr::VARIABLE fn_available(const expr::FUNCTION_ARGS& args);

	static expr::VARIABLE fn_capacity_pretty(const expr::FUNCTION_ARGS& args);
	static expr::VARIABLE fn_used_pretty(const expr::FUNCTION_ARGS& args);
	static expr::VARIABLE fn_free_pretty(const expr::FUNCTION_ARGS& args);
	static expr::VARIABLE fn_available_pretty(const expr::FUNCTION_ARGS& args);
};
