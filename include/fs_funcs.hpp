#pragma once

#include <string>

namespace fs {

	bool exists(const std::string& filename);
	bool is_file(const std::string& filename);
	bool is_accessible(const std::string& filename);

	bool is_readable(const std::string& filename);
}
