#pragma once

#include <mutex>
#include <thread>
#include "common.hpp"
#include "lowercase_map.hpp"
#include "config.hpp"
#include "expr/expression.hpp"
#include "plugin.hpp"

class plugin::MEMINFO : public plugin::PLUGIN {

	public:

		virtual const std::string type() const override { return "meminfo"; }

		explicit MEMINFO(CONFIG::MAP *cfg);
		~MEMINFO();

	static expr::VARIABLE fn_meminfo_ram_total(const expr::FUNCTION_ARGS& args);
	static expr::VARIABLE fn_meminfo_ram_used(const expr::FUNCTION_ARGS& args);
	static expr::VARIABLE fn_meminfo_ram_free(const expr::FUNCTION_ARGS& args);

	static expr::VARIABLE fn_meminfo_swap_total(const expr::FUNCTION_ARGS& args);
	static expr::VARIABLE fn_meminfo_swap_used(const expr::FUNCTION_ARGS& args);
	static expr::VARIABLE fn_meminfo_swap_free(const expr::FUNCTION_ARGS& args);

};
