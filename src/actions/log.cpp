#include "common.hpp"
#include "logger.hpp"
#include "actions/log.hpp"

void action::LOG::execute(const std::vector<expr::RESULT>& args) {

	std::string s;

	for ( auto& arg : args ) {

		if ( !s.empty()) s += ' ';

		if ( arg.is_string()) s += arg.to_string();
		else s += common::to_string(arg.to_double());
	}

	if ( !s.empty())
		logger::info["action"] << logger::unique() << s << std::endl;
}
