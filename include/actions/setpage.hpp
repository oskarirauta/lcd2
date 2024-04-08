#pragma once

#include "action.hpp"

class action::SETPAGE : public action::ACTION {

	public:

		virtual const std::string cmd()  const override { return "page::set"; }

		virtual void execute(const std::vector<expr::RESULT>& args) override;
};
