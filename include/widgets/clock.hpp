#pragma once

#include <chrono>

#include "common.hpp"
#include "lowercase_map.hpp"
#include "config.hpp"
#include "widget.hpp"

class widget::CLOCK : public widget::WIDGET {

	protected:

		bool _needs_draw = false;
		bool render();

	public:
		virtual const std::string type() const override { return "clock"; }
		virtual bool update() override;

		explicit CLOCK(const std::string& name, CONFIG::MAP *cfg);
		~CLOCK();
};
