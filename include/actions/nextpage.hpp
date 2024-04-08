#pragma once

#include "action.hpp"

class action::NEXTPAGE : public action::ACTION {

	public:

		virtual const std::string cmd()  const override { return "page::next"; }

		virtual void execute(const std::vector<expr::RESULT>& args) override;
};
