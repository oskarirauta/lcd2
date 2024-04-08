#pragma once

#include <string>
#include <map>

#include "expr/expression.hpp"
#include "widget_classes.hpp"
#include "plugin.hpp"

class LAYOUT {

	private:
		std::vector<int> parse_page_sequence(const std::string& s);

	public:

		struct WIDGET_LINK {

			std::string name;
			int x, y;

			widget::WIDGET* get_ptr();
			bool reloads();
			bool update();

			WIDGET_LINK() : name("unknown"), x(0), y(0) {}
			WIDGET_LINK(const std::string& name, int x, int y) : name(name), x(x), y(y) {}
			WIDGET_LINK(const std::string& key, const std::string& value);
			~WIDGET_LINK() {}
		};

		struct LAYER {

			int number;
			std::vector<LAYOUT::WIDGET_LINK> widgets;

			LAYER() : number(-1) {}
			LAYER(int n) : number(n) {}
			~LAYER() {
				this -> widgets.clear();
			}
		};

		struct PAGE {

			int number;
			std::map<int, LAYER> layers;
			std::vector<std::string> timers;
			std::string on_enter;
			std::string on_exit;

			void update_widgets();
			bool enter();
			bool exit();

			PAGE() : number(-1) {}
			PAGE(int n) : number(n) {}
			~PAGE() {
				this -> layers.clear();
			}
		};

		std::map<int, PAGE> pages;
		std::vector<std::string> timers;
		std::vector<int> page_sequence;
		int prev_page_index = -1;
		int next_page_index = 0;
		int default_page = 0;

		const std::string type() { return "config"; }
		const std::string name() { return "layout"; }

		const std::string dump();
		void render(int *forced_page = nullptr, bool all = false);
		bool update();

		LAYOUT();
		LAYOUT(CONFIG::MAP *cfg);
		~LAYOUT();

		static std::string page_name(const int& page_no);
		static expr::VARIABLE fn_page(const expr::FUNCTION_ARGS& args);
};
