#include "common.hpp"
#include "config.hpp"
#include "properties.hpp"

PROPERTIES::PROPERTIES() {

	this -> _properties.clear();
	this -> property = expr::PROPERTY(&this -> _properties, &CONFIG::functions, &CONFIG::variables);
}

PROPERTIES::~PROPERTIES() {

	this -> _properties.clear();
}

const std::string PROPERTIES::P2S(const std::string& key, const std::string& def) {

	return this -> _properties.contains(key) ? this -> property[key, def].to_string() : def;
}

double PROPERTIES::P2N(const std::string& key, double def) {

	return this -> _properties.contains(key) ? this -> property[key, def].to_double() : def;
}

int PROPERTIES::P2I(const std::string& key, int def) {

	return this -> _properties.contains(key) ? this -> property[key, (double)def].to_int() : def;
}

bool PROPERTIES::P2B(const std::string& key, bool def) {

	if ( !this -> _properties.contains(key))
		return def;

	double _def = def ? 1 : 0;
	expr::RESULT res = this -> property[key, _def];

	if ( res.is_number()) {
		return res.to_int() == 0 ? false : true;
	} else if ( res.is_string() && common::is_number(res.to_string())) {
		return res.to_string() == "0" ? false : true;
	} else if ( res.is_string() && (
		common::to_lower(res.to_string()) == "yes" ||
		common::to_lower(res.to_string()) == "true"
	)) {
		return true;
	} else if ( res.is_string() && (
		common::to_lower(res.to_string()) == "no" ||
		common::to_lower(res.to_string()) == "false"
	)) {
		return false;
	}

	return def;
}

expr::RESULT PROPERTIES::P2RES(const std::string& key, const std::variant<double, std::string, std::nullptr_t> def) {

	return this -> _properties.contains(key) ? this -> property[key, def] : expr::RESULT(def);
}
