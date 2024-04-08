#pragma once

#include <mutex>
#include <thread>
#include "common.hpp"
#include "lowercase_map.hpp"
#include "config.hpp"
#include "expr/expression.hpp"
#include "plugin.hpp"

class plugin::TEST : public plugin::PLUGIN {

	public:

		virtual const std::string type() const override { return "test"; }

		explicit TEST(CONFIG::MAP *cfg);
		~TEST();

	static expr::VARIABLE fn_on_off(const expr::FUNCTION_ARGS& args);

};
