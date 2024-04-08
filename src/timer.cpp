#include <algorithm>
#include "common.hpp"
#include "throws.hpp"
#include "logger.hpp"

#include "config.hpp"

#include "plugin_classes.hpp"
#include "widget_classes.hpp"
#include "expr/expression.hpp"
#include "display.hpp"
#include "layout.hpp"
#include "action.hpp"
#include "timer.hpp"

TIMER::TIMER() {

	this -> _properties = {
		{ "active", "1" },
		{ "update", "1500" },
	};
}

TIMER::TIMER(const std::string& name, CONFIG::MAP *cfg) {

	this -> _name = common::unquoted(common::to_lower(common::trim_ws(std::as_const(name))));

	this -> _properties = {
		{ "active", "1" },
		{ "update", "1500" },
	};

	if ( name.empty())
		throws << "failed to create timer, timer is missing name" << std::endl;

	if ( cfg == nullptr || cfg -> empty())
		throws << "failed to create timer '" << name << "', configuration is missing" << std::endl;

	std::vector<std::string> allowed_keys = {
		"active", "condition", "action", "interval"
	};

	for ( auto& [k, v] : *cfg ) {

		std::string key = k == "update" ? "interval" : k;

		if ( key == "interval" ) {

			if ( std::holds_alternative<std::string>(v)) this -> _properties["interval"] = std::get<std::string>(v);
			else logger::error["config"] << "timer interval cannot be object or array" << std::endl;
		}

		if ( k.starts_with("expression") && k != "expressions" && k.size() > 10 ) {

			std::string str(k);
			str.erase(0, 10);

			if ( !common::is_number(str)) {

				logger::error["config"] << "syntax error for timer " << name << ", key " << k << " is not supported" << std::endl;
				continue;
			}
		}

		if ( k.starts_with("expression") && std::holds_alternative<CONFIG::VECTOR>(v)) {

			auto vec = std::get<CONFIG::VECTOR>(v);

			size_t empties = 0;
			for ( const std::string& str : vec ) {

				if ( str.empty()) {
					empties++;
					continue;
				}

				std::string expr_name = this -> new_expression_name();
				this -> _properties[expr_name] = str;
			}

			if ( vec.empty() || vec.size() == empties )
				logger::warning["config"] << "found " << k << " expressions array for timer " << name << ", but it is empty" << std::endl;

			continue;

		} else if ( k.starts_with("expression") && std::holds_alternative<std::string>(v)) {

			std::string str = std::get<std::string>(v);
			if ( str.empty()) {

				logger::warning["config"] << "found " << k << " for timer " << name << ", but it is empty" << std::endl;
				continue;
			}

			std::string expr_name = this -> new_expression_name();
			this -> _properties[expr_name] = str;
			continue;

		} else if ( k.starts_with("expression")) {

			logger::error["config"] << "unsupported value type for timer " << name << " with key " << k << std::endl;
			continue;
		}

		std::string value = std::get<std::string>(v);

		if ( !CONFIG::parse_option("timer", key, value, &allowed_keys))
			continue;

		if ( key == "expression" ) {

			std::string expr_name = this -> new_expression_name();
			this -> _properties[key] = value;
		}

		this -> _properties[key] = value;
	}

	if ( this -> expressions().empty() &&
		( !this -> _properties.contains("action") || this -> _properties["action"].empty()))
		throws << "timer '" << name << "' failed to initialize, either expressions array or action is required" << std::endl;

	this -> _properties["interval"] = std::to_string(this -> interval());
}

const std::string TIMER::new_expression_name() const {

	int i = -1;

	for ( auto p : this -> _properties ) {

		if ( p.first.starts_with("expression")) {

			std::string raw_s = p.first;
			std::string s = raw_s;
			s = s.substr(10, s.size() - 10);
			s = common::trim_ws(common::unquoted(common::trim_ws(std::as_const(s))));

			if ( !s.empty() && common::is_number(s) && s.find_first_of('.') == std::string::npos ) {

				try {
					int v = std::stoi(s);
					if ( v > i ) i = v;
				} catch ( std::exception &e ) {
					logger::debug["config"] << "problem parsing number of expression name, name was '" <<
						raw_s << "'" << std::endl;
				}

			} else if ( !common::is_number(s))
				logger::warning["config"] << "timer '" << this -> _name << "' expression cannot parse number from '" << raw_s << "'" << std::endl;
		}
	}

	return "expression" + std::to_string(i + 1);
}

const std::vector<std::string> TIMER::expressions() const {

	std::vector<std::string> vec;

	for ( auto p : this -> _properties ) {
		if ( p.first.starts_with("expression"))
			vec.push_back(p.second);
	}
	return vec;
}

const std::string TIMER::type() const {
	return "timer";
}

const std::string TIMER::name() const {
	return this -> _name;
}

TIMER& TIMER::operator =(const TIMER& other) {

	this -> _name = other._name;
	this -> last_updated = other.last_updated;

	for ( auto& [k, v] : other._properties )
		this -> _properties[k] = v;

	return *this;
}

int TIMER::interval() {

	if ( auto _p = this -> property["interval"]; _p.is_number()) {

		int i = _p.to_int();
		if ( i < 50 ) i = 50;
		else if ( i > 3000 ) i = 3000;
		return i;

	} else return 1500;
}

bool TIMER::active() {

	if ( auto _p = this -> property["active"]; _p.is_number())
		return _p.to_int() == 0 ? false : true;
	else return true;
}

const std::string TIMER::get_action() const {

	for ( const auto& [k, v] : this -> _properties )
		if ( k == "action" && !v.empty())
			return v;

	return "";
}

const bool TIMER::is_global(LAYOUT* layout) const {

	return std::find(layout -> timers.begin(), layout -> timers.end(), this -> _name) != layout -> timers.end();
}

const bool TIMER::on_page(LAYOUT* layout, int page_no) const {

	if ( !layout -> pages.contains(page_no))
		return false;

	return std::find(layout -> pages[page_no].timers.begin(), layout -> pages[page_no].timers.end(), this -> _name) !=
			layout -> pages[page_no].timers.end();
}


void TIMER::evaluate(const std::string& expr) {

	if ( expr.empty()) {
		logger::error["timer"] << "failed to evaluate timer expression, expression was empty" << std::endl;
		return;
	}

	expr::expression e(expr);
	std::string pretty = e.operator std::string();

	try {

		expr::RESULT result = e.evaluate(&CONFIG::functions, &CONFIG::variables);
		//logger::debug["timer"] << "evaluated expression '" << pretty << "' result: " << result << std::endl;

	} catch ( std::runtime_error &err ) {

		logger::error["timer"] << "failed to evaluate expression '" << pretty << "', reason: " << err.what() << std::endl;

	}
}

bool TIMER::update() {

	std::chrono::milliseconds next = this -> last_updated + std::chrono::milliseconds(this -> interval());
	std::chrono::milliseconds now = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());

	if ( now < next )
		return false;

	this -> last_updated = now;
	if ( !this -> active())
		return false;

	if (( !this -> _properties.contains("expression") || this -> _properties["expression"].empty()) &&
		( !this -> _properties.contains("action") || this -> _properties["action"].empty()) &&
		this -> expressions().empty()) {

		logger::warning["timer"] << "timer " << this -> _name << " has no expressions or action, disabling it" << std::endl;
		this -> _properties["active"] = "0";

		return false;
	}

	if (( this -> _properties.contains("expression") && !this -> _properties["expression"].empty()) || !this -> expressions().empty()) {

		for ( auto expr : this -> expressions())
			this -> evaluate(expr);
	}

	if ( this -> _properties.contains("condition") && this -> _properties.contains("action") &&
		!this -> _properties["condition"].empty() && !this -> _properties["action"].empty()) {

		if ( auto _p = this -> property["condition"]; _p.is_number() && _p.to_int() != 0 )
			display -> actions -> execute(this -> _properties["action"], "timer::" + this -> _name);

	} else if (( !this -> _properties.contains("condition") || this -> _properties.contains("condition")) &&
		this -> _properties["condition"].empty() && this -> _properties.contains("action") &&
		!this -> _properties["action"].empty()) {

			display -> actions -> execute(this -> _properties["action"], "timer::" + this -> _name);
	}

	return true;
}

const std::string TIMER::dump() {

	std::stringstream ss;
	ss << "timer " << this -> _name << " {\n";

	ss << "\tactive\t";
	if ( this -> _properties.contains("active") && !this -> _properties["active"].empty())
		ss << this -> _properties["active"] << "\n";
	else ss << "1" << "\n";

	ss << "\texpressions [" << "\n";

	for ( const std::string& expr : this -> expressions())
		ss << "\t\t" << expr << "\n";
	ss << "\t]\n";

	if ( this -> _properties.contains("action") && !this -> _properties["action"].empty()) {

		if ( this -> _properties.contains("condition") && !this -> _properties["condition"].empty())
			ss << "\tcondition\t" << this -> _properties["condition"] << "\n";

		ss << "\taction\t" << this -> _properties["action"] << "\n";
	}

	ss << "\tinterval\t";
	if ( this -> _properties.contains("interval") && !this -> _properties["interval"].empty())
		ss << this -> _properties["interval"] << "\n";
	else ss << "1500" << "\n";

	ss << "}";

	return ss.str();
}

std::ostream& operator <<(std::ostream& os, TIMER const& t) {

	os << "timer '" << t._name << "' {\n";

	std::vector<std::string> vec = t.expressions();
	if ( !vec.empty() || !t.get_action().empty()) {

		os << common::padding(2) << "expressions [" << "\n";

		for ( const std::string& expr : vec )
			os << common::padding(6) << expr << "\n";

		os << common::padding(2) << "]\n";
	}

	for ( const auto& [k, v] : t._properties ) {

		if ( k.starts_with("expression"))
			continue;
		else
			os << common::padding(2) << k << common::padding(20 - k.size()) << v << "\n";
	}

	os << "}";
	return os;
}

std::ostream& operator <<(std::ostream& os, TIMER const *t) {

	os << *t;
	return os;
}
