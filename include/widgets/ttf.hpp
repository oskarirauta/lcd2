#pragma once

#include <chrono>

#include "common.hpp"
#include "lowercase_map.hpp"
#include "config.hpp"
#include "widget.hpp"

class widget::TTF : public widget::WIDGET {

	protected:

		bool _needs_draw = false;
		bool render(const std::string& text, const std::string& font);

	public:
		virtual const std::string type() const override { return "ttf"; }
		virtual bool update() override;

		explicit TTF(const std::string& name, CONFIG::MAP *cfg);
		~TTF();
};
