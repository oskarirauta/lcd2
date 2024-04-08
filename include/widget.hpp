#pragma once

#include <string>
#include <utility>
#include <chrono>
#include <vector>

#include "common.hpp"
#include "config.hpp"
#include "properties.hpp"
#include "lowercase_map.hpp"
#include "expr/expression.hpp"
#include "rgb.hpp"

class DISPLAY;

class widget {

	public:
		class IMAGE;
		class TTF;
		class LINECHART;
		class BAR;

		class WIDGET : public PROPERTIES {

			friend class DISPLAY;

			protected:
				std::string _name;
				int _width = 0;
				int _height = 0;
				int _pwidth = 0;
				int _pheight = 0;
				bool _needs_update = false;
				bool _needs_draw = false;
				int _interval = -1;
				char _use_cycles = -1;
				int _cycle = -1;
				bool _was_visible = false;

				std::chrono::milliseconds last_updated = std::chrono::milliseconds(0);

			public:
				std::vector<RGBA> bitmap;

				virtual const std::string name() const;
				virtual const std::string type() const = 0;

				virtual bool reloads();
				virtual bool use_cycles();
				virtual int interval();
				virtual int width() const;
				virtual int height() const;
				virtual int previous_width() const;
				virtual int previous_height() const;
				virtual bool center();
				virtual bool visible();
				virtual bool needs_draw() const;
				virtual bool update() = 0;
				virtual bool time_to_update();

				WIDGET();
				virtual ~WIDGET();

				friend std::ostream& operator <<(std::ostream& os, widget::WIDGET const& w);
				friend std::ostream& operator <<(std::ostream& os, widget::WIDGET const *w);
		};

		static std::vector<std::string> types;

		common::lowercase_map<std::shared_ptr<widget::WIDGET>> widgets;

		common::lowercase_map<std::shared_ptr<widget::WIDGET>>::iterator begin();
		common::lowercase_map<std::shared_ptr<widget::WIDGET>>::iterator end();
		common::lowercase_map<std::shared_ptr<widget::WIDGET>>::size_type size();

		bool contains(const std::string& name);
		void erase(const std::string& name);
		bool empty();

		void add(const std::string& name, CONFIG::MAP *cfg);

		widget::WIDGET* operator [](const std::string& name);

		~widget();

		static unsigned char convert_alpha(unsigned char gdAlpha);

		friend std::ostream& operator <<(std::ostream& os, widget const& w);
		friend std::ostream& operator <<(std::ostream& os, widget const *w);
};

std::ostream& operator <<(std::ostream& os, widget::WIDGET const& w);
std::ostream& operator <<(std::ostream& os, widget::WIDGET const *w);
std::ostream& operator <<(std::ostream& os, widget const& w);
std::ostream& operator <<(std::ostream& os, widget const *w);
