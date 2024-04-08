#pragma once

#include <chrono>

#include "common.hpp"
#include "lowercase_map.hpp"
#include "config.hpp"
#include "widget.hpp"

class widget::BAR : public widget::WIDGET {

	private:

		int value;

	protected:

		bool _needs_draw = false;
		bool render();
		int smoother(int value, int smooth);
		bool value_did_change();

	public:
		virtual const std::string type() const override { return "image"; }
		virtual bool update() override;

		explicit BAR(const std::string& name, CONFIG::MAP *cfg);
		~BAR();
};
