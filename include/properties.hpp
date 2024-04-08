#pragma once

#include <string>
#include "expr/expression.hpp"

class PROPERTIES {

	protected:
		expr::PROPERTYMAP _properties;

	public:
		expr::PROPERTY property;

		const std::string P2S(const std::string& key, const std::string& def = "");
		double P2N(const std::string& key, double def = (double)0);
		int P2I(const std::string& key, int def = 0);
		bool P2B(const std::string& key, bool def = false);
		expr::RESULT P2RES(const std::string& key, const std::variant<double, std::string, std::nullptr_t> def = nullptr);

		PROPERTIES();
		~PROPERTIES();
};
