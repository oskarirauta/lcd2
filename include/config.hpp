#pragma once

#include <fstream>
#include <string>
#include <vector>

#include "tsl/ordered_map.h"
#include "rva/variant.hpp"
#include "expr/expression.hpp"

class CONFIG {

	friend class DISPLAY;

	public:

	using NODE = rva::variant<
		std::string,
		std::vector<std::string>,
		tsl::ordered_map<std::string, rva::self_t>
	>;

	using MAP = tsl::ordered_map<std::string, NODE>;
	using VECTOR = std::vector<std::string>;

	static bool parse_option(const std::string& section, std::string key, std::string value, std::vector<std::string>* allowed = nullptr);
	static bool evaluate_string(const std::string& section, const std::string& key, const std::string& expr, std::string& value, bool to_lower);
	static bool evaluate_double(const std::string& section, const std::string& key, const std::string& expr, double& value);
	static bool evaluate_int(const std::string& section, const std::string& key, const std::string& expr, int& value);
	static bool evaluate_result(const std::string& section, const std::string& key, const std::string& expr, expr::RESULT &result);

	private:

	std::ifstream fd;

	void load();
	void parse();

	protected:

	std::string _filename;
	MAP _cfg;

	public:

	static expr::VARIABLEMAP variables;
	static expr::FUNCTIONMAP functions;

	MAP::iterator begin();
	MAP::iterator end();
	MAP::size_type size();

	bool contains(const std::string& name);
	void erase(const std::string& name);
	bool empty();

	NODE* operator [](const std::string& name);

	CONFIG(const std::string& filename);
	~CONFIG();

	friend std::ostream& operator <<(std::ostream& os, CONFIG const& c);
	friend std::ostream& operator <<(std::ostream& os, CONFIG const *c);
};

std::ostream& operator <<(std::ostream& os, CONFIG const& c);
std::ostream& operator <<(std::ostream& os, CONFIG const *c);
