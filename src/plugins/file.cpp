#include <fstream>
#include <filesystem>

#include "logger.hpp"
#include "throws.hpp"
#include "plugin.hpp"
#include "plugins/file.hpp"

expr::VARIABLE plugin::FILE::fn_readline(const expr::FUNCTION_ARGS& args) {

        std::string filename = !args.empty() && args[0].string_convertible().empty()?
		common::trim_ws(args[0].to_string()) : "";

	if ( filename.empty()) {

		logger::error["plugin"] << "readline needs filename as argument" << std::endl;
		return "";

	} else if ( !std::filesystem::exists(filename)) {

		logger::error["plugin"] << "readline cannot open " << filename << " - file does not exist" << std::endl;
		return "";
	}

	int line_no = 1;

	if ( args.size() > 1 ) {

		std::string s = args[1].string_convertible().empty() ?
			common::trim_ws(common::unquoted(common::trim_ws(args[1].to_string()))) : "";

		if ( !s.empty()) {

			try {
				line_no = std::stoi(s);
			} catch ( const std::exception& e ) {

				line_no = 1;
				logger::error << "readline cannot parse line number from " << s <<std::endl;

			}
		}
	}

	std::ifstream fd(filename);
	if ( !fd.good()) {

		logger::error["plugin"] << "readline failed to open " << filename << " - file exists but is not readable, permission problem?" << std::endl;
		if ( fd.is_open())
			fd.close();

		return "";
	}

	std::string str;
	while ( std::getline(fd, str) && line_no > 0 )
		line_no--;

	if ( line_no > 0 )
		logger::warning["plugin"] << "readline failed to parse line, file does not have enough lines" << std::endl;

	return str;
}

expr::VARIABLE plugin::FILE::fn_readconf(const expr::FUNCTION_ARGS& args) {

	if ( args.empty() || !args[0].string_convertible().empty()) {

		logger::error["plugin"] << "readconf needs filename as argument" << std::endl;
		return "";
	}

	std::string filename = common::trim_ws(args[0].to_string());
	std::string fallback = "";

	if ( args.size() < 2 || !args[1].string_convertible().empty()) {

		logger::error["plugin"] << "readconf needs key name as second argument" << std::endl;
		return "";
	}

	std::string key = common::to_lower(common::trim_ws(common::unquoted(common::trim_ws(args[1].to_string()))));

	if ( args.size() > 2 ) {

		if ( !args[2].string_convertible().empty()) {

			logger::warning["plugin"] << "readconf's 3rd argument, fallback value, is not convertible to string, ignoring" << std::endl;

		} else fallback = args[2].to_string();

	}

	if ( !std::filesystem::exists(filename)) {

		logger::error["plugin"] << "readconf cannot open " << filename << " - file does not exist" << std::endl;
		return fallback;
	}

	std::string value;

	std::ifstream fd(filename);
	if ( !fd.good()) {

		logger::error["plugin"] << "readconf failed to open " << filename << " - file exists but is not readable, permission problem?" << std::endl;
		if ( fd.is_open())
			fd.close();

		return fallback;
	}

	std::string str;
	while ( std::getline(fd, str)) {

		str = common::trim_ws(common::unquoted(common::trim_ws(str)));

		if ( str.starts_with(key)) {

			str.erase(0, key.size());
			while ( common::whitespace.contains(str.front()))
				str.erase(0, 1);
			value = common::trim_ws(common::unquoted(common::trim_ws(str)));
			break;
		}
	}

	fd.close();
	return value.empty() ? fallback : value;
}

expr::VARIABLE plugin::FILE::fn_exists(const expr::FUNCTION_ARGS& args) {

	if ( args.empty() || !args[0].string_convertible().empty()) {

		logger::error["plugin"] << "readline needs filename as argument" << std::endl;
		return "";
	}

	std::string filename = common::trim_ws(args[0].to_string());

	return std::filesystem::exists(filename);
}

plugin::FILE::FILE(CONFIG::MAP *cfg) {

	logger::vverbose["plugin"] << "initializing plugin " << this -> type() << std::endl;
	CONFIG::functions.append({ "file::readline", plugin::FILE::fn_readline });
	CONFIG::functions.append({ "file::readconf", plugin::FILE::fn_readconf });
	CONFIG::functions.append({ "file::exists", plugin::FILE::fn_exists });

}

plugin::FILE::~FILE() {

	CONFIG::functions.erase("file::readline");
	CONFIG::functions.erase("file::readconf");
	CONFIG::functions.erase("file::exists");

}
