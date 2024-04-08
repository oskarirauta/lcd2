#include "logger.hpp"
#include "throws.hpp"
#include "plugin.hpp"
#include "process.hpp"
#include "plugins/exec.hpp"

static std::pair<std::string, std::vector<std::string>> parse_cmd(const std::string& s) {

	std::string cmd;
	std::vector<std::string> args;
	std::string raw(common::trim_ws(std::as_const(s)));
	std::string word;

	if ( s.empty())
		return { cmd, args };

	char prev = 0, quote = 0;

	while ( !raw.empty()) {

		if ( raw.front() == ' ' && word.empty()) {
			prev = raw.front();
			raw.erase(0, 1);
			continue;
		}

		if ( raw.front() == ' ' && quote == 0 ) {

			prev = raw.front();
			raw.erase(0, 1);
			word = common::trim_ws(common::unquoted(common::trim_ws(std::as_const(word))));

			if ( !word.empty() && cmd.empty())
				cmd = word;
			else if ( !word.empty() && !cmd.empty())
				args.push_back(word);

			word = "";
			continue;
		}

		if ( quote == 0 && prev != '\\' && raw.front() == '"' ) {
			prev = raw.front();
			quote = raw.front();
			word += raw.front();
			raw.erase(0, 1);
			continue;
		} else if ( quote == 0 && prev != '\\' && raw.front() == '\'' ) {
			prev = raw.front();
			quote = raw.front();
			word += raw.front();
			raw.erase(0, 1);
			continue;
		} else if ( quote != 0 && prev != '\\' && raw.front() == quote ) {
			prev = raw.front();
			quote = 0;
			word += raw.front();
			raw.erase(0, 1);
			continue;
		}

		prev = raw.front();
		word += raw.front();
		raw.erase(0, 1);

	}

	if ( !word.empty()) {

		if ( quote != 0 ) {

			logger::warning["plugin"] << "exec command-line with un-even quotes, added missing quote to end" << std::endl;
			word += quote;
		}

		word = common::trim_ws(common::unquoted(common::trim_ws(std::as_const(word))));

		if ( !word.empty()) {

			if ( cmd.empty())
				cmd = word;
			else args.push_back(word);
		}
	}

	return { cmd, args };
}

expr::VARIABLE plugin::EXEC::fn_exec(const expr::FUNCTION_ARGS& args) {

	if ( args.empty()) {
		logger::error["plugin"] << "exec without arguments; exec plugin needs command to execute" << std::endl;
		return "";
	} else if ( !args[0].string_convertible().empty()) {
		logger::error["plugin"] << "exec failed, argument not usable" << std::endl;
		return "";
	}

	std::pair<std::string, std::vector<std::string>>cmd = parse_cmd(args[0].to_string());

	if ( cmd.first.empty()) {
		logger::error["plugin"] << "exec failed to parse command-line" << std::endl;
		return "";
	}

	process_t *proc = new process_t(cmd.first, cmd.second);
	std::string result = *proc;
	proc -> status();
	delete proc;

	return result;
}

plugin::EXEC::EXEC(CONFIG::MAP *cfg) {

	logger::vverbose["plugin"] << "plugin " << this -> type() << " initialized" << std::endl;
	CONFIG::functions.append({ "exec", plugin::EXEC::fn_exec });
}

plugin::EXEC::~EXEC() {

	CONFIG::functions.erase("exec");
}
