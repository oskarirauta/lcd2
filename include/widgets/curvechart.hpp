#pragma once

#include <vector>

#include "common.hpp"
#include "lowercase_map.hpp"
#include "config.hpp"
#include "widget.hpp"

class widget::CURVECHART : public widget::WIDGET {

	private:

		std::vector<unsigned char> _values;
		size_t _num_samples = 0;
		int next_value = 0;

	protected:

		bool _needs_draw = false;
		bool render();
		int smoother(int value, int smooth);

	public:
		virtual const std::string type() const override { return "curvechart"; }
		virtual bool update() override;

		explicit CURVECHART(const std::string& name, CONFIG::MAP *cfg);
		~CURVECHART();
};
