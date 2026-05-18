#pragma once

#include <chrono>

#include "common.hpp"
#include "lowercase_map.hpp"
#include "config.hpp"
#include "widget.hpp"

class widget::GAUGE : public widget::WIDGET {

	private:

		int value = 0;

	protected:

		bool _needs_draw = false;
		bool render();
		bool value_did_change();

	public:
		virtual const std::string type() const override { return "gauge"; }
		virtual bool update() override;

		explicit GAUGE(const std::string& name, CONFIG::MAP *cfg);
		~GAUGE();
};
