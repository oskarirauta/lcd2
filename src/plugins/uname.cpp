#include <cstdint>

#include <string.h>
#include <sys/utsname.h>
#include "logger.hpp"
#include "throws.hpp"
#include "plugin.hpp"
#include "plugins/uname.hpp"

expr::VARIABLE plugin::UNAME::fn_sysname(const expr::FUNCTION_ARGS& args) {

	if ( !args.empty())
		logger::warning["plugin"] << "plugin uname does not accept arguments" << std::endl;

	utsname utsbuf;
	if ( ::uname(&utsbuf) != 1 )
		return std::string(utsbuf.sysname);
	else {
		logger::error["plugin"] << "plugin uname failed to get values, reason: " << std::string(strerror(errno)) << std::endl;
		return "";
	}
}

expr::VARIABLE plugin::UNAME::fn_nodename(const expr::FUNCTION_ARGS& args) {

	if ( !args.empty())
		logger::warning["plugin"] << "plugin uname does not accept arguments" << std::endl;

	utsname utsbuf;
	if ( ::uname(&utsbuf) != 1 )
		return std::string(utsbuf.nodename);
	else {
		logger::error["plugin"] << "plugin uname failed to get values, reason: " << std::string(strerror(errno)) << std::endl;
		return "";
	}
}

expr::VARIABLE plugin::UNAME::fn_release(const expr::FUNCTION_ARGS& args) {

	if ( !args.empty())
		logger::warning["plugin"] << "plugin uname does not accept arguments" << std::endl;

	utsname utsbuf;
	if ( ::uname(&utsbuf) != 1 )
		return std::string(utsbuf.release);
	else {
		logger::error["plugin"] << "plugin uname failed to get values, reason: " << std::string(strerror(errno)) << std::endl;
		return "";
	}
}

expr::VARIABLE plugin::UNAME::fn_version(const expr::FUNCTION_ARGS& args) {

	if ( !args.empty())
		logger::warning["plugin"] << "plugin uname does not accept arguments" << std::endl;

	utsname utsbuf;
	if ( ::uname(&utsbuf) != 1 )
		return std::string(utsbuf.version);
	else {
		logger::error["plugin"] << "plugin uname failed to get values, reason: " << std::string(strerror(errno)) << std::endl;
		return "";
	}
}

expr::VARIABLE plugin::UNAME::fn_machine(const expr::FUNCTION_ARGS& args) {

	if ( !args.empty())
		logger::warning["plugin"] << "plugin uname does not accept arguments" << std::endl;

	utsname utsbuf;
	if ( ::uname(&utsbuf) != 1 )
		return std::string(utsbuf.machine);
	else {
		logger::error["plugin"] << "plugin uname failed to get values, reason: " << std::string(strerror(errno)) << std::endl;
		return "";
	}
}

#if defined(_GNU_SOURCE)
expr::VARIABLE plugin::UNAME::fn_domainname(const expr::FUNCTION_ARGS& args) {

	if ( !args.empty())
		logger::warning["plugin"] << "plugin uname does not accept arguments" << std::endl;

	utsname utsbuf;
	if ( ::uname(&utsbuf) != 1 )
		return std::string(utsbuf.domainname);
	else {
		logger::error["plugin"] << "plugin uname failed to get values, reason: " << std::string(strerror(errno)) << std::endl;
		return "";
	}
}
#else
expr::VARIABLE plugin::UNAME::fn_domainname(const expr::FUNCTION_ARGS& args) {

	logger::error["plugin"] << "uname::domainname is not supported by this system" << std::endl;
	return "";
}
#endif

plugin::UNAME::UNAME(CONFIG::MAP *cfg) {

	logger::vverbose["plugin"] << "initializing plugin uname" << std::endl;

	CONFIG::functions.append({ "uname::sysname", plugin::UNAME::fn_sysname });
	CONFIG::functions.append({ "uname::nodename", plugin::UNAME::fn_nodename });
	CONFIG::functions.append({ "uname::hostname", plugin::UNAME::fn_nodename });
	CONFIG::functions.append({ "uname::release", plugin::UNAME::fn_release });
	CONFIG::functions.append({ "uname::kernel", plugin::UNAME::fn_release });
	CONFIG::functions.append({ "uname::version", plugin::UNAME::fn_version });
	CONFIG::functions.append({ "uname::build", plugin::UNAME::fn_version });
	CONFIG::functions.append({ "uname::machine", plugin::UNAME::fn_machine });
	CONFIG::functions.append({ "uname::domainname", plugin::UNAME::fn_domainname });

}

plugin::UNAME::~UNAME() {

        CONFIG::functions.erase("uname::sysname");
	CONFIG::functions.erase("uname::nodename");
	CONFIG::functions.erase("uname::hostname");
	CONFIG::functions.erase("uname::release");
	CONFIG::functions.erase("uname::kernel");
	CONFIG::functions.erase("uname::version");
	CONFIG::functions.erase("uname::build");
	CONFIG::functions.erase("uname::machine");
	CONFIG::functions.erase("uname::domainname");
}
