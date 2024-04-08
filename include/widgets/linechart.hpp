#pragma once

#include <chrono>
#include <deque>

#include "common.hpp"
#include "lowercase_map.hpp"
#include "config.hpp"
#include "widget.hpp"

class widget::LINECHART : public widget::WIDGET {

	private:

		//std::deque<unsigned char> _values;
		std::vector<unsigned char> _values;
		size_t _longest_width = 0;
		unsigned char next_value;

	protected:

		bool _needs_draw = false;
		bool render();
		int smoother(int value, int smooth);

	public:
		virtual const std::string type() const override { return "image"; }
		virtual bool update() override;

		explicit LINECHART(const std::string& name, CONFIG::MAP *cfg);
		~LINECHART();
};
