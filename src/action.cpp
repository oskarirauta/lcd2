#include "common.hpp"
#include "logger.hpp"
#include "config.hpp"
#include "action.hpp"
#include "actions/log.hpp"
#include "actions/setpage.hpp"
#include "actions/prevpage.hpp"
#include "actions/nextpage.hpp"

action::action() {

	this -> actions.push_back(std::make_shared<action::LOG>());
	this -> actions.push_back(std::make_shared<action::SETPAGE>());
	this -> actions.push_back(std::make_shared<action::NEXTPAGE>());
	this -> actions.push_back(std::make_shared<action::PREVPAGE>());

}

action::~action() {

	this -> actions.clear();
}

void action::execute(const std::string& s, const std::string& source) {

	std::string _s = s;
	std::string _cmd;
	std::string _args;
	size_t pos;
	if ( pos = _s.find_first_of('('); pos == std::string::npos ) {

		logger::error["action"] << "syntax error, invalid action declaration '" << s << "'" << std::endl;
		logger::vverbose["action"] << "actions are always like functions such as action_name('arg1', 5) or setpage(3)" << std::endl;
		return;
	}

	_cmd = _s.substr(0, pos);
	_cmd = common::trim_ws(_cmd);
	_s.erase(0, pos);
	_s = common::trim_ws(_s);

	if ( _s.empty() || _cmd.empty() || _s.front() != '(' || _s.back() != ')') {

		logger::error["action"] << "syntax error, invalid action declaration '" << s << "'" << std::endl;
		logger::vverbose["action"] << "actions are always like functions such as action_name('arg1', 5) or setpage(3)" << std::endl;
		return;
	}

	_s.erase(0, 1);
	_s.pop_back();
	_args = common::trim_ws(_s);

	std::vector<std::shared_ptr<action::ACTION>>::iterator it;

	for ( it = this -> actions.begin(); it != this -> actions.end(); it++ )
		if ( it -> get() -> cmd() == _cmd )
			break;

	if ( it == actions.end()) {

		logger::error["action"] << "unknown action '" << _cmd << "'" <<
				( source.empty() ? "" : ( " executed by " + source )) << std::endl;
		return;
	}

	std::vector<std::string> strings;
	std::string arg;
	char quote = 0;
	int parentheses = 0;

	for ( auto& ch : _args ) {

		if ( quote == 0 && std::string("\n\r\f\v").contains(ch))
			continue;
		else if ( quote == 0 && ch == '\t' ) ch = ' ';

		if ( arg.empty() && ch == ' ' )
			continue;

		if ( quote == 0 && parentheses == 0 && ch == ',' ) {

			strings.push_back(arg);
			arg = "";
			continue;
		}

		if ( quote == 0 && ch == '(' ) parentheses++;
		else if ( quote == 0 && ch == ')' && parentheses > 0 ) parentheses--;
		else if ( quote == 0 && ( ch == '\'' || ch == '"' )) quote = ch;
		else if ( quote != 0 && ch == quote ) quote = 0;

		arg += ch;
	}

	if ( !arg.empty()) {

		if ( quote != 0 ) arg += quote;
		while ( parentheses > 0 ) {
			arg += ')';
			parentheses--;
		}

		strings.push_back(arg);
	}

	std::vector<expr::RESULT> args;

	for ( const std::string& s : strings ) {

		if ( s.empty()) {

			args.push_back(expr::VARIABLE(""));
			continue;
		}

		try {
			expr::expression e(s);
			expr::RESULT r = e.evaluate(&CONFIG::functions, &CONFIG::variables);
			args.push_back(r);
		} catch ( std::runtime_error& e ) {

			args.push_back(expr::VARIABLE(""));
		}
	}

	it -> get() -> execute(args);

}
