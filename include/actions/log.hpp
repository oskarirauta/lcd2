#pragma once

#include "action.hpp"

class action::LOG : public action::ACTION {

	public:

		virtual const std::string cmd()  const override { return "log"; }
		virtual void execute(const std::vector<expr::RESULT>& args) override;
};
