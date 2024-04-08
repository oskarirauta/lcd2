#include <stdexcept>
#include <fstream>
#include <sstream>
#include <cstring>
#include <cerrno>
#include <unistd.h>
#include <sys/stat.h>
#include <variant>
#include <iomanip>
#include <algorithm>
#include <filesystem>

#include "lowercase_map.hpp"
#include "common.hpp"
#include "logger.hpp"
#include "throws.hpp"
#include "plugin_classes.hpp"
#include "widget_classes.hpp"
#include "display.hpp"
#include "timer.hpp"
#include "layout.hpp"
#include "config.hpp"

expr::VARIABLEMAP CONFIG::variables;
expr::FUNCTIONMAP CONFIG::functions;

static size_t line_no = 0;
static size_t unnamed_cnt = 0;

static std::string dump_cfg(const CONFIG::MAP& m, int level) {

	if ( m.empty())
		return "";

	std::stringstream ss;

	for ( auto [k, v] : m ) {

		if ( auto pos = k.find_first_of(':'); pos != std::string::npos &&
			( std::holds_alternative<CONFIG::MAP>(v) ||
			std::holds_alternative<CONFIG::VECTOR>(v))) {

			std::string name(k.substr(pos + 1, k.size() - pos));

			if ( !name.empty()) {
				ss << common::padding(level * 2) <<
					k.substr(0, pos) << ( level == 0 ? " '" : " " ) <<
					name << ( level == 0 ? "' " : " ");

			} else ss << common::padding(level * 2) << k << " ";
		} else ss << common::padding(level * 2) << k << " ";

		if ( std::holds_alternative<std::string>(v)) {

			ss << common::padding(20 - k.size()) << " " << std::get<std::string>(v) << "\n";

		} else if ( std::holds_alternative<CONFIG::MAP>(v)) {

			ss << "{" << "\n";
			ss << dump_cfg(std::get<CONFIG::MAP>(v), level + 1);
			ss << common::padding(level * 2) << "}" << "\n" << ( level == 0 ? "\n" : "" );

		} else if ( std::holds_alternative<CONFIG::VECTOR>(v)) {

			ss << "[" << "\n";

			for ( auto& n : std::get<CONFIG::VECTOR>(v))
				ss << common::padding(( level + 1 ) * 2) << n << "\n";

			ss << common::padding(level * 2) << "]" << "\n";
		}
	}

	return ss.str();
}

static void is_accessible(const std::string& filename) {

	struct stat st;

	if ( stat(filename.c_str(), &st) == -1 )
		throw std::runtime_error("stat(" + filename + ") failed: " + std::string(std::strerror(errno)));

	if ( !( S_ISCHR(st.st_mode) && filename == "/dev/null" ) && !S_ISREG(st.st_mode))
		throw std::runtime_error(filename + " is not a regular file");

	if ( st.st_uid != geteuid() || st.st_gid != getegid())
		throw std::runtime_error("owner and/or group of '" + filename + "' don't match");
}

static std::string trim_first_word(const std::string& s, std::string& value) {

	std::string ss(s);
	value = "";
	ss = common::ltrim_ws(ss);

	if ( ss.empty())
		return "";

	if ( ss.front() == '\'' || ss.front() == '"' ) {
		auto quote = ss.front();
		ss.erase(0, 1);
		if ( ss.front() == quote ) value = "";
		else if ( auto pos = ss.find_first_of(quote); pos != std::string::npos ) {
			value = "";
			value += std::string(ss.substr(0, pos));
			while ( common::whitespace.find(value.back()) != std::string::npos )
				value.pop_back();
		}

		if ( ss.size() >= value.size() + 2 )
			ss.erase(0, value.size() + 2);
		else ss = "";
		return ss;

	} else if ( auto pos = ss.find_first_of(common::whitespace); pos != std::string::npos ) {
		std::string ret(ss.substr(pos + 1, ss.size() - (pos + 1)));
		if ( !ret.empty()) {
			value = std::string(ss.substr(0, ss.size() - ret.size()));
			while ( common::whitespace.find(value.back()) != std::string::npos )
				value.pop_back();
		}
		return ret;
	}

	value = "";
	return "";
}

static std::string quickfix_value(const std::string& s) { // remove comments at end of value if value is simple, such as quoted string or word or number

	std::string ss(common::ltrim_ws(std::as_const(s)));

	if ( ss.empty() || ss.front() == '#' )
		return "";

	std::string res;

	if ( std::isdigit(ss.front()) || ( ss.size() > 1 && ss.front() == '.' && std::isdigit(ss.at(1)))) { // number value

		if ( !ss.empty() && ss.front() == '.' ) {
			res = "0.";
			ss.erase(0, 1);
		}

		while ( !ss.empty() && std::isdigit(ss.front())) {

			res += ss.front();
			ss.erase(0, 1);

			if ( ss.front() == '.' && ss.size() == 1 ) {

				ss.erase(0, 1);

			} else if ( ss.front() == '.' && ss.size() > 1 && std::isdigit(ss.at(1))) {

				res += '.';
				ss.erase(0, 1);

				continue;
			}

		}

		if ( !res.empty() && !ss.empty() && common::whitespace.find_first_of(ss.front()) != std::string::npos ) {

			ss = common::ltrim_ws(ss);

			if ( ss.empty() || ss.front() == '#' )
				return res;
			else
				return common::ltrim_ws(std::as_const(s));

		} else if ( !res.empty() && ss.empty())
			return res;

	} else if ( ss.front() == '\'' || ss.front() == '"' ) { // quoted string

		auto quote = ss.front();
		ss.erase(0, 1);

		while ( !ss.empty() && ss.front() != quote ) {

			if ( ss.front() == '\\' && ss.size() > 1 && ss.at(1) == quote ) {

				res += '\\' + quote;
				ss.erase(0, 2);
				continue;
			}

			res += ss.at(0);
			ss.erase(0, 1);
		}

		if ( !res.empty() && !ss.empty() && ss.front() == quote ) {

			ss.erase(0, 1); // remove quote

			if ( !ss.empty()) {

				if ( ss.front() == '#' )
					return common::ltrim_ws(std::as_const(s));

				ss = common::ltrim_ws(ss);
			}

			if ( ss.empty() || ss.front() == '#' ) {
				res = quote + res + quote;
				return res;
			} else
				return common::ltrim_ws(std::as_const(s));
		}

	} else if ( common::is_alpha(ss.front())) { // word without quotes

		while ( !ss.empty() && common::whitespace.find_first_of(ss.front()) == std::string::npos && common::is_alnum(ss.front())) {

			res += ss.front();
			ss.erase(0, 1);

		}

		if ( !res.empty() && !ss.empty() && common::whitespace.find_first_of(ss.front()) != std::string::npos ) {

			ss = common::ltrim_ws(ss);

			if ( ss.empty() || ss.front() == '#' )
				return res;
			else
				return common::ltrim_ws(std::as_const(s));
		}
	}

	return common::ltrim_ws(std::as_const(s));

}

static std::string trim_comment_from_end(const std::string& line) {

	size_t pos = line.size();
	std::vector<char> quotes;
	char prev = 0;
	std::string s = line;

	for ( size_t i = 0; i < line.size(); i++ ) {

		if ( s.at(i) == ' ' && i + 1 < line.size() && s.at(i + 1) == '#' && quotes.empty()) {
			pos = i;
			break;
		}

		if (( s.at(i) == '\'' || s.at(i) == '"' ) && prev != '\\' && !quotes.empty() && quotes.back() == s.at(i))
			quotes.pop_back();
		else if (( s.at(i) == '\'' || s.at(i) == '"' ) && prev != '\\' && ( quotes.empty() || quotes.back() != s.at(i)))
			quotes.push_back(s.at(i));

		prev = s.at(i);
	}

	//std::cout << std::endl;

	if ( pos < line.size()) {
		s = s.substr(0, pos);
		s = common::trim_ws(s);
	}

	return s;

}

// TODO: check un-even quotes

static void parse_array(std::ifstream& fd, CONFIG::VECTOR& cfg, bool full_lines) {

	std::string line;
	std::string sep = common::whitespace + ",";

	while ( std::getline(fd, line)) {

		line_no++;

		std::string name;

		if ( line = common::ltrim_ws(line, sep); line.empty() || line.front() == '#' )
			continue;

		if ( full_lines ) {

			if ( line.front() == ']' )
				break;

			line = common::trim_ws(line);
			line = trim_comment_from_end(line);
			if ( !line.empty() && line.front() != '#' )
				cfg.push_back(line);
			continue;
		}

		while ( !line.empty()) {

			auto quote = line.front();
			if ( quote == '\'' || quote == '"' )
				line.erase(0, 1);
			else quote = 0;

			while ( !line.empty() && (( quote != 0 && line.front() != quote ) ||
				( quote == 0 && (sep + "#]").find_first_of(line.front()) == std::string::npos ))) {

				name += line.front();
				line.erase(0, 1);
			}

			if ( quote != 0 && line.front() == quote )
				line.erase(0, 1);

			name = common::unquoted(name);
			name = common::trim_ws(name);

			if ( !name.empty()) {
				cfg.push_back(name);
				name = "";
			}

			if ( line = common::ltrim_ws(line, sep); line.front() == '#' || line.front() == ']' )
				break;
		}

		if ( line.front() == ']' )
			break;

	}

	if ( line.front() != ']' )
		throw std::runtime_error("un-even brackets, ']' missing at end of array");
}

// TODO: check un-even braces and parentheses

static void parse_cfg(std::ifstream& fd, CONFIG::MAP& cfg, bool root) {

	std::string line;

	while ( std::getline(fd, line)) {

		line_no++;
		std::string name, value;

		if ( line = common::ltrim_ws(line); line.empty())
			continue;

		while ( !line.empty() && common::whitespace.find(line.front()) == std::string::npos ) {
			name += line.front();
			line.erase(0, 1);
		}

		if ( name.front() == '#' )
			continue; // comment line

		name = common::to_lower(common::unquoted(name));

		if ( !root && name.front() == '}' ) {

			value.erase(0, 1);

			if ( !value.empty()) {
				if ( value = common::trim_ws(value); !value.empty() && value.front() != '#' )
					logger::warning["config"] << "line " + line_no << ": garbage after '}' <" << value << ">" << std::endl;
			}

			break;
		}

		if ( name.empty()) {

			if ( fd.is_open())
				fd.close();

			throw std::runtime_error("line " + std::to_string(line_no) + ": empty section name, fatal error");
		}

		line = common::ltrim_ws(line);

		if ( name == "include" ) {

			value = line;
			if ( !value.empty()) {

				std::string::value_type quote = 0;
				bool was_whitespace = false;
				bool escaped = false;

				for ( int i = 0; i < (int)value.size(); i++ ) {

					if (( value[i] == '\'' || value[i] == '"' ) && !escaped && quote == 0 ) {
						quote = value[i];
						continue;
					} else if ( quote != 0 && value[i] == quote && !escaped ) {

						quote = 0;
						continue;
					} else if ( value[i] == '\\' && !escaped ) {

						escaped = true;
						continue;
					} else escaped = false;

					if ( quote == 0 && was_whitespace && value[i] == '#' ) {

						value = value.substr(0, i - 1);
						break;

					} else if ( quote == 0 && common::whitespace.find_first_of(value[i]) != std::string::npos )
						was_whitespace = true;
					else was_whitespace = false;
				}
			}

			if ( !root ) {

				logger::error["config"] << "invalid include directive in config, include can only be used on the root level" << std::endl;
				continue;
			} else if ( value.empty()) {

				logger::error["config"] << "invalid include directive in config, filename not specified" << std::endl;
				continue;
			} else if ( !std::filesystem::exists(value)) {

				logger::error["config"] << "ignoring include directive, file " << value << " does not exists" << std::endl;
				continue;
			}

			try { is_accessible(value); }
			catch ( const std::exception& e ) {

				logger::error["config"] << "ignoring include directive, file " << value << " is not accessible" << std::endl;
				logger::vverbose["config"] << "permission problem with file " << value << " maybe?" << std::endl;
				continue;
			}

			std::ifstream _fd(value, std::ios::in);
			if ( _fd.fail() || !_fd.is_open() || !_fd.good()) {

				logger::error["config"] << "failed to read included file " << value << std::endl;
				if ( !_fd.good())
					logger::vverbose << "permission problem with file " << value << " maybe?" << std::endl;

				if ( _fd.is_open())
					_fd.close();

				continue;
			}

			logger::verbose["config"] << "including file " << value << std::endl;

			parse_cfg(_fd, cfg, true);

			if ( _fd.is_open())
				_fd.close();
			continue;
		}


		if ( std::string last_part = trim_first_word(line, value);
			last_part.front() == '{' || line.front() == '{' || last_part.front() == '[' || line.front() == '[' ) {

			bool is_array = line.front() == '[' || last_part.front() == '[';

			if ( line.front() == '{' || line.front() == '[' ) {
				value = "";
				last_part = line;
			}

			value = common::trim_ws(value);
			last_part.erase(0, 1);

			if ( !last_part.empty()) {

				if ( last_part = common::trim_ws(last_part); !last_part.empty() && last_part.front() != '#' )
					logger::warning["config"] << "line " + line_no << ": garbage after '{' <" << last_part << ">" << std::endl;

			}

			if ( !value.empty()) {

				value = common::to_lower(value);

				std::string err;
				while ( !value.empty()) {

					if ( auto pos = value.find_first_of(":"); pos != std::string::npos ) {
						if ( err.empty())
							err = ( is_array ? "array '" : "section '" ) + value + "' contains illegal ':' characters";
						value.erase(pos, 1);
					} else break;
				}


				if ( !value.empty()) {

					if ( !err.empty())
						logger::warning["config"] << "line " << line_no << ": " << err << " trimmed to " << value << std::endl;

					name += ":" + value;
				}
			}

			if ( root && value.empty() && (
				name == "plugin" || name == "timer" || name == "widget" )) {

				value = "unnamed" + std::to_string(++unnamed_cnt);
				while ( cfg.contains("widget:" + value))
					value = "unnamed" + std::to_string(++unnamed_cnt);

				logger::warning["config"] << "line " << line_no << ": defines " << name << " without name, name is required for " <<
					name << "s, using '" << value << "'" << std::endl;
				logger::verbose["config"] << "this propably will not be very useful, but atleast it won't break config" << std::endl;

				name += ":" + value;
			}

			size_t this_line_no = line_no;

			if ( is_array ) {

				CONFIG::VECTOR arr;

				try {
					parse_array(fd, arr, common::has_prefix(name, "timers") ? false : true);
				} catch ( const std::runtime_error& e) {

					if ( fd.is_open())
						fd.close();

					throw std::runtime_error("line " + std::to_string(this_line_no) + ": " +
						name + ( name.empty() ? "" : " " ) +
						( value.empty() ? "" : ( "'" + value + "' " )) +
						( value.empty() && name.empty() ? "array " : "" ) +
						"parsing failure: " + e.what());
				}

				if ( cfg.contains(name))
					logger::warning["config"] << "line " << this_line_no << ": " <<
						name << ( name.empty() ? "" : " " ) <<
						( value.empty() ? "" : ( "'" + value + "' " )) <<
						( value.empty() && name.empty() ? "array/object " : "" ) <<
						" already defined, array ignored" << std::endl;
				else if ( arr.empty())
					logger::debug["config"] << "line " << this_line_no << ": " <<
						name << ( name.empty() ? "" : " " ) <<
						( value.empty() ? "" : ( "'" + value + "' " )) <<
						( value.empty() && name.empty() ? "array/object " : "" ) <<
						" ignored, array is empty" << std::endl;
				else if ( !arr.empty())
					cfg[name] = arr;

			} else {

				CONFIG::MAP section;

				try {
					parse_cfg(fd, section, false);
				} catch ( const std::runtime_error& e ) {

					if ( fd.is_open())
						fd.close();

					throw e;
				}

				if ( cfg.contains(name))
					logger::warning["config"] << "line " << this_line_no << ": " <<
						name << ( name.empty() ? "" : " " ) <<
						( value.empty() ? "" : ( "'" + value + "' " )) <<
						( value.empty() && name.empty() ? "array/object " : "" ) <<
						" already defined, array ignored" << std::endl;
				else if ( section.empty())
					logger::debug["config"] << "line " << this_line_no << ": " <<
						name << ( name.empty() ? "" : " " ) <<
						( value.empty() ? "" : ( "'" + value + "' " )) <<
						( value.empty() && name.empty() ? "array/object " : "" ) <<
						" ignored, array is empty" << std::endl;
				else if ( !section.empty())
					cfg[name] = section;
			}

			continue;

		} else value = "";

		while ( !line.empty()) {
			value += line.front();
			line.erase(0, 1);
		}

		value = quickfix_value(value);

		if ( value.empty() || value.front() == '#' ) {

			logger::warning["config"] << "line " << line_no << ": ignoring '" << name << "' with empty value" << std::endl;
			continue;
		}

		cfg[name] = value;
	}

	if ( root && fd.is_open())
		fd.close();
}

void CONFIG::parse() {

	if ( !this -> fd.is_open())
		throw std::runtime_error("cannot read config file" + ( this -> _filename.empty() ? "" : ( " " + this -> _filename )) +
			", file is not open");

	line_no = 0;
	unnamed_cnt = 0;
	this -> _cfg.clear();
	this -> fd.seekg(0);

	try {
		parse_cfg(this -> fd, this -> _cfg, true);
	} catch ( const std::runtime_error& e ) {

		if( this -> fd.is_open())
			this -> fd.close();

		throw e;
	}

	if ( this -> fd.is_open())
		this -> fd.close();
}

/*
// this could be handy..
static bool valid_chars(std::string& line, const bool num_begin) {

	int idx = -1;
	for ( char &ch : line ) {

		idx++;

		if (( ch >= 'A' && ch <= 'Z' ) || ( ch >= 'a' && ch <= 'z' ) || ch == '_' )
			continue;

		if (( num_begin || idx > 0 ) && ch >= '0' && ch <= '9' )
			continue;

		if ( idx > 0 && ( ch == '.' || ch == '_' ))
			continue;

		return false;
	}

	return true;
}
*/



void CONFIG::load() {

	try { is_accessible(this -> _filename); }
	catch ( const std::exception& e ) { throws << e.what() << std::endl; }

	this -> fd.open(this -> _filename, std::ios::in);

	if ( this -> fd.fail() || !this -> fd.is_open() || !this -> fd.good()) {

		if ( !this -> fd.good()) {

			if ( this -> fd.is_open())
				this -> fd.close();

			throw std::runtime_error("failed to read file '" + this -> _filename + "', possible permission problem");
		}

		if ( this -> fd.is_open())
			this -> fd.close();

		throw std::runtime_error("failed to read file '" + this -> _filename + "'");
	}
}

CONFIG::MAP::iterator CONFIG::begin() {
	return this -> _cfg.begin();
}

CONFIG::MAP::iterator CONFIG::end() {
	return this -> _cfg.end();
}

CONFIG::MAP::size_type CONFIG::size() {
	return this -> _cfg.size();
}

bool CONFIG::contains(const std::string& name) {
	return this -> _cfg.contains(name);
}

void CONFIG::erase(const std::string& name) {

	if ( this -> _cfg.contains(name))
		this -> _cfg.erase(name);
}

bool CONFIG::empty() {
	return this -> _cfg.empty();
}

CONFIG::NODE* CONFIG::operator [](const std::string& name) {

	if ( !this -> _cfg.contains(name))
		return nullptr;

	return &(this -> _cfg[name]);
}

CONFIG::CONFIG(const std::string& filename) {

	this -> _filename = filename;

	try {
		this -> load();
		this -> parse();
	} catch ( const std::runtime_error& e ) {
		throw e;
	}

	if ( this -> fd.is_open())
		this -> fd.close();
}

CONFIG::~CONFIG() {

	if ( this -> fd.is_open())
		this -> fd.close();
}

bool CONFIG::parse_option(const std::string& section, std::string key, std::string value, std::vector<std::string>* allowed) {

	if ( key = common::trim_ws(common::to_lower(key)); key.empty()) {

		logger::error["config"] << "empty key in section " << section << std::endl;
		return false;
	}

	value = common::trim_ws(value);

	if ( allowed != nullptr && std::find(allowed -> begin(), allowed -> end(), key) == allowed -> end()) {

		logger::warning["config"] << "unknown key '" << key << "' for " <<
			section << ", allowed keys are: " << common::join_vector(*allowed) << std::endl;

		return false;

	} else if ( value.empty()) {

		logger::verbose["config"] << "note: '" << key << "' in " <<
			section << " section does not have a value" << std::endl;

		return false;
	}

	return true;
}

bool CONFIG::evaluate_string(const std::string& section, const std::string& key, const std::string& expr, std::string& value, bool to_lower) {

	expr::expression e(expr);

	try {
		expr::RESULT result = e.evaluate(&functions, &variables);

		if ( result.operator std::string().empty()) {

			logger::verbose["config"] << "note: ignoring '" << key << "' in section " << section << ", it evaluated to empty value" << std::endl;
			return false;

		} else value = to_lower ? common::to_lower(result.operator std::string()) : result.operator std::string();

	} catch ( std::runtime_error &e ) {

		logger::error["config"] << "evaluation failure for '" << key << "' in section " << section << ": " << e.what() << std::endl;
		return false;
	}

	return true;
}

bool CONFIG::evaluate_double(const std::string& section, const std::string& key, const std::string& expr, double& value) {

	expr::expression e(expr);

	try {
		expr::RESULT result = e.evaluate(&functions, &variables);
		value = result.operator double();

	} catch ( std::runtime_error &e ) {

		logger::error["config"] << "evaluation failure for '" << key << "' in section " << section << ": " << e.what() << std::endl;
		return false;
	}

	return true;
}

bool CONFIG::evaluate_int(const std::string& section, const std::string& key, const std::string& expr, int& value) {

	double d;
	if ( !CONFIG::evaluate_double(section, key, expr, d))
		return false;

	value = (int)d;
	return true;
}

bool CONFIG::evaluate_result(const std::string& section, const std::string& key, const std::string& expr, expr::RESULT& value) {

	expr::expression e(expr);

	try {
		expr::RESULT result = e.evaluate(&functions, &variables);
		value = result.operator double();

	} catch ( std::runtime_error &e ) {

		logger::error["config"] << "evaluation failure for '" << key << "' in section " << section << ": " << e.what() << std::endl;
		return false;
	}

	return true;
}

std::ostream& operator <<(std::ostream& os, CONFIG const& c) {

	os << dump_cfg(c._cfg, 0);
	return os;
}

std::ostream& operator <<(std::ostream& os, CONFIG const *c) {

	os << dump_cfg(c -> _cfg, 0);
	return os;
}
