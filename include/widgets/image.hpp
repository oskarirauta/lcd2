#pragma once

#include <chrono>

#include "common.hpp"
#include "lowercase_map.hpp"
#include "config.hpp"
#include "widget.hpp"

class widget::IMAGE : public widget::WIDGET {

	protected:

		bool _needs_draw = false;
		bool render(const std::string &filename);

	public:
		virtual const std::string type() const override { return "image"; }
		virtual bool update() override;

		explicit IMAGE(const std::string& name, CONFIG::MAP *cfg);
		~IMAGE();
};
