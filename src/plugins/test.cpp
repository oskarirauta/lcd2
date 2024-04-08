#include "logger.hpp"
#include "throws.hpp"
#include "plugin.hpp"
#include "plugins/test.hpp"

expr::VARIABLE plugin::TEST::fn_on_off(const expr::FUNCTION_ARGS& args) {

	bool value = true;

	if ( args.empty())

		logger::warning["plugin"] << "plugin test did not receive arg, assuming true" << std::endl;

	else if ( !args[0].is_number_convertible()) {

		logger::warning["plugin"] << "plugin test arg is not bool or number convertible, assuming true" << std::endl;

	} else value = args[0].to_int() == 0 ? false : true;

	return !value;
}

plugin::TEST::TEST(CONFIG::MAP *cfg) {

	logger::vverbose["plugin"] << "initializing plugin test" << std::endl;

	CONFIG::functions.append({ "test::on_off", plugin::TEST::fn_on_off });

}

plugin::TEST::~TEST() {

        CONFIG::functions.erase("test::on_off");
}
