#pragma once

#include "action.hpp"

class action::PREVPAGE : public action::ACTION {

	public:

		virtual const std::string cmd()  const override { return "page::prev"; }

		virtual void execute(const std::vector<expr::RESULT>& args) override;
};
