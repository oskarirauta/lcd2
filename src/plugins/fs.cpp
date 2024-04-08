#include <filesystem>

#include "logger.hpp"
#include "throws.hpp"
#include "plugin.hpp"
#include "plugins/fs.hpp"

expr::VARIABLE plugin::FS::fn_capacity(const expr::FUNCTION_ARGS& args) {

	if ( args.empty() || !args[0].is_string_convertible() || args[0].to_string().empty()) {

		logger::error["plugin"] << "plugin fs needs path where filesystem is mounted or filename, " <<
			( args.empty() ? "none provided" : ( !args[0].is_string_convertible() ? "argument is not string" : "argument is empty" )) <<
			std::endl;
		return (double)0;
	}

	std::string path = args[0].to_string();
	std::error_code ec;

	if ( !std::filesystem::exists(path) || ( !std::filesystem::is_directory(path) && !std::filesystem::is_regular_file(path))) {

		logger::error["plugin"] << "path provided for plugin fs, does not exist and is not directory or regular file" << std::endl;
		return (double)0;
	}

	if ( std::filesystem::is_regular_file(path)) {

		if ( const std::uintmax_t size = std::filesystem::file_size(path, ec); ec ) {

			logger::error["plugin"] << "plugin fs cannot retrieve size for " << path << ", reason: " << ec.message() << std::endl;
			return (double)0;

		} else return (double)size;
	}

	if ( const std::filesystem::space_info si = std::filesystem::space(path, ec); ec ) {

		logger::error["plugin"] << "plugin fs cannot retrieve space info for " << path << ", reason: " << ec.message() << std::endl;
		return (double)0;
	} else return (double)si.capacity;
}

expr::VARIABLE plugin::FS::fn_capacity_pretty(const expr::FUNCTION_ARGS& args) {

	return common::HumanReadable(plugin::FS::fn_capacity(args).to_double());
}

expr::VARIABLE plugin::FS::fn_used(const expr::FUNCTION_ARGS& args) {

	if ( args.empty() || !args[0].is_string_convertible() || args[0].to_string().empty()) {

		logger::error["plugin"] << "plugin fs needs path where filesystem is mounted, " <<
			( args.empty() ? "argument not given" : ( !args[0].is_string_convertible() ? "argument is not string" : "argument is empty" )) <<
			std::endl;
		return (double)0;
	}

	std::string path = args[0].to_string();
	std::error_code ec;

	if ( !std::filesystem::exists(path) || !std::filesystem::is_directory(path)) {

		logger::error["plugin"] << "path provided for plugin fs, does not exist or is not directory" << std::endl;
		return (double)0;
	}

	if ( const std::filesystem::space_info si = std::filesystem::space(path, ec); ec ) {

		logger::error["plugin"] << "plugin fs cannot retrieve space info for " << path << ", reason: " << ec.message() << std::endl;
		return (double)0;

	} else return (double)(si.capacity - si.free);
}

expr::VARIABLE plugin::FS::fn_used_pretty(const expr::FUNCTION_ARGS& args) {

	return common::HumanReadable(plugin::FS::fn_used(args).to_double());
}

expr::VARIABLE plugin::FS::fn_free(const expr::FUNCTION_ARGS& args) {

	if ( args.empty() || !args[0].is_string_convertible() || args[0].to_string().empty()) {

		logger::error["plugin"] << "plugin fs needs path where filesystem is mounted, " <<
			( args.empty() ? "argument not given" : ( !args[0].is_string_convertible() ? "argument is not string" : "argument is empty" )) <<
			std::endl;
		return (double)0;
	}

	std::string path = args[0].to_string();
	std::error_code ec;

	if ( !std::filesystem::exists(path) || !std::filesystem::is_directory(path)) {

		logger::error["plugin"] << "path provided for plugin fs, does not exist or is not directory" << std::endl;
		return (double)0;
	}

	if ( const std::filesystem::space_info si = std::filesystem::space(path, ec); ec ) {

		logger::error["plugin"] << "plugin fs cannot retrieve space info for " << path << ", reason: " << ec.message() << std::endl;
		return (double)0;

	} else return (double)si.free;
}

expr::VARIABLE plugin::FS::fn_free_pretty(const expr::FUNCTION_ARGS& args) {

	return common::HumanReadable(plugin::FS::fn_free(args).to_double());
}

expr::VARIABLE plugin::FS::fn_available(const expr::FUNCTION_ARGS& args) {

	if ( args.empty() || !args[0].is_string_convertible() || args[0].to_string().empty()) {

		logger::error["plugin"] << "plugin fs needs path where filesystem is mounted, " <<
			( args.empty() ? "argument not given" : ( !args[0].is_string_convertible() ? "argument is not string" : "argument is empty" )) <<
			std::endl;
		return (double)0;
	}

	std::string path = args[0].to_string();
	std::error_code ec;

	if ( !std::filesystem::exists(path) || !std::filesystem::is_directory(path)) {

		logger::error["plugin"] << "path provided for plugin fs, does not exist or is not directory" << std::endl;
		return (double)0;
	}

	if ( const std::filesystem::space_info si = std::filesystem::space(path, ec); ec ) {

		logger::error["plugin"] << "plugin fs cannot retrieve space info for " << path << ", reason: " << ec.message() << std::endl;
		return (double)0;

	} else return (double)si.available;
}

expr::VARIABLE plugin::FS::fn_available_pretty(const expr::FUNCTION_ARGS& args) {

	return common::HumanReadable(plugin::FS::fn_available(args).to_double());
}

plugin::FS::FS(CONFIG::MAP *cfg) {

	logger::vverbose["plugin"] << "initializing plugin fs" << std::endl;

	CONFIG::functions.append({ "fs::size", plugin::FS::fn_capacity });
	CONFIG::functions.append({ "fs::capacity", plugin::FS::fn_capacity });
	CONFIG::functions.append({ "fs::used", plugin::FS::fn_used });
	CONFIG::functions.append({ "fs::free", plugin::FS::fn_free });
	CONFIG::functions.append({ "fs::available", plugin::FS::fn_available });

	CONFIG::functions.append({ "fs::size::hr", plugin::FS::fn_capacity_pretty });
	CONFIG::functions.append({ "fs::capacity::hr", plugin::FS::fn_capacity_pretty });
	CONFIG::functions.append({ "fs::used::hr", plugin::FS::fn_used_pretty });
	CONFIG::functions.append({ "fs::free::hr", plugin::FS::fn_free_pretty });
	CONFIG::functions.append({ "fs::available::hr", plugin::FS::fn_available_pretty });
}

plugin::FS::~FS() {

        CONFIG::functions.erase("fs::size");
	CONFIG::functions.erase("fs::capacity");
	CONFIG::functions.erase("fs::used");
	CONFIG::functions.erase("fs::free");
	CONFIG::functions.erase("fs::available");

	CONFIG::functions.erase("fs::size::hr");
	CONFIG::functions.erase("fs::size::hr");
	CONFIG::functions.erase("fs::size::hr");
	CONFIG::functions.erase("fs::size::hr");
	CONFIG::functions.erase("fs::size::hr");
}
