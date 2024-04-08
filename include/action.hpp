#pragma once

#include <string>
#include <vector>
#include "expr/expression.hpp"

class action {

	public:

		class LOG;
		class SETPAGE;
		class PREVPAGE;
		class NEXTPAGE;

		class ACTION {

			public:

				virtual const std::string cmd() const = 0;

				virtual void execute(const std::vector<expr::RESULT>& args) = 0;
		};

		std::vector<std::shared_ptr<action::ACTION>> actions;

		void execute(const std::string& s, const std::string& source = "");

		action();
		~action();
};
