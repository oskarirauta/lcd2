#include <iostream>
#include <memory>

#include "common.hpp"
#include "logger.hpp"
#include "plugin_classes.hpp"

common::lowercase_map<bool> plugin::types = { };

plugin::PLUGIN::PLUGIN() {
}

plugin::PLUGIN::~PLUGIN() {
}

bool plugin::PLUGIN::enabled() {
	return this -> _enabled;
}

bool plugin::PLUGIN::update() {
	return true;
}

int plugin::PLUGIN::interval() {

	if ( auto _p = this -> property["interval"]; _p.is_number()) {

		int i = _p.to_int();
		return ( i != 0 && i < 500 ? 500 : i);

	} else return 1500;
}

bool plugin::is_configurable(const std::string& name) {
	return plugin::types.contains(name) ? plugin::types[name] : false;
}

void plugin::add(const std::string& name, CONFIG::MAP *cfg) {

	std::string _name = common::unquoted(common::to_lower(common::trim_ws(std::as_const(name))));

	if ( _name.find_first_not_of("abcdefghijklmnopqrstuvwxyz1234567890_") != std::string::npos ||
		!std::isalpha(_name.front())) {

		logger::error["config"] << "illegal name '" << _name << "' for plugin, names must begin " <<
			"with alphabetical character and can only contain characters " <<
			"from set 'abcdefghijklmnopqrstuvwxyz1234567890_'" << std::endl;
		return;
	} else if ( _name.empty()) {

		logger::error["config"] << "syntax error, cannot create widget without name" << std::endl;
		return;
	}

	if ( common::is_any_of(name, { "exec", "cpuinfo", "meminfo", "netinfo", "file", "test", "uname", "fs", "uptime" })) {

		logger::notice["config"] << "plugin " << name << " does not have anything to configure" << std::endl;
		return;
	}

	if ( !plugin::types.contains(_name)) {

		logger::error["config"] << "unsupported plugin configuration '" << _name << "', ignored" << std::endl;
		return;
	}

	if ( this -> plugins.contains(_name)) {

		logger::error["config"] << "plugin '" << _name << "' already configured, ignoring configuration" << std::endl;
		return;
	}

/*
	try {
	} catch ( const std::exception& e ) {
	}
*/

}

common::lowercase_map<std::shared_ptr<plugin::PLUGIN>>::iterator plugin::begin() {
	return this -> plugins.begin();
}

common::lowercase_map<std::shared_ptr<plugin::PLUGIN>>::iterator plugin::end() {
	return this -> plugins.end();
}

common::lowercase_map<std::shared_ptr<plugin::PLUGIN>>::size_type plugin::size() {
	return this -> plugins.size();
}

common::lowercase_map<std::shared_ptr<plugin::PLUGIN>>::const_iterator plugin::begin() const {
	return this -> plugins.begin();
}

common::lowercase_map<std::shared_ptr<plugin::PLUGIN>>::const_iterator plugin::end() const {
	return this -> plugins.end();
}

common::lowercase_map<std::shared_ptr<plugin::PLUGIN>>::size_type plugin::size() const {
	return this -> plugins.size();
}

bool plugin::contains(const std::string& name) {
	return this -> plugins.contains(name);
}

void plugin::erase(const std::string& name) {

	if ( this -> plugins.contains(name))
		this -> plugins.erase(name);
}

bool plugin::empty() {
	return this -> plugins.empty();
}

plugin::PLUGIN* plugin::operator [](const std::string& name) {
	return !this -> plugins.contains(name) ? nullptr : this -> plugins[name].get();
}

plugin::plugin() {

	this -> plugins["exec"] = std::make_shared<plugin::EXEC>(nullptr);
	this -> plugins["cpuinfo"] = std::make_shared<plugin::CPUINFO>(nullptr);
	this -> plugins["meminfo"] = std::make_shared<plugin::MEMINFO>(nullptr);
	this -> plugins["netinfo"] = std::make_shared<plugin::NETINFO>(nullptr);
	this -> plugins["fs"] = std::make_shared<plugin::FS>(nullptr);
	this -> plugins["file"] = std::make_shared<plugin::FILE>(nullptr);
	this -> plugins["uname"] = std::make_shared<plugin::UNAME>(nullptr);
	this -> plugins["uptime"] = std::make_shared<plugin::UPTIME>(nullptr);
	this -> plugins["test"] = std::make_shared<plugin::TEST>(nullptr);

}

plugin::~plugin() {
	this -> plugins.clear();
}
