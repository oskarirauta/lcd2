#include <filesystem>
#include <fstream>

#include "fs_funcs.hpp"

bool fs::exists(const std::string& filename) {
	return std::filesystem::exists(filename);
}

bool fs::is_file(const std::string& filename) {
	return std::filesystem::is_regular_file(filename);
}

bool fs::is_accessible(const std::string& filename) {
	std::ifstream f(filename.c_str());
	return f.good();
}

bool fs::is_readable(const std::string& filename) {
	return fs::exists(filename) && fs::is_file(filename) &&
		fs::is_accessible(filename);
}
