#include "common.hpp"
#include "logger.hpp"
#include "display.hpp"
#include "layout.hpp"
#include "actions/setpage.hpp"

void action::SETPAGE::execute(const std::vector<expr::RESULT>& args) {

	if ( args.empty()) {

		logger::error["action"] << "no arguments for setpage action, page number is needed as argument" << std::endl;
		return;

	} else if ( !args[0].is_number_convertible()) {

		logger::error["action"] << "argument '" << args[0].to_string() << "' is not valid, argument must be number" << std::endl;
		return;
	}

	int page_no = args[0].to_int();

	if ( page_no < 0 ) {

		logger::warning["action"] << "setpage action can accept only positive numbers, " << page_no << " is converted to " <<
				std::abs(page_no) << std::endl;
		page_no = std::abs(page_no);
	}

	if ( !display -> layout -> pages.contains(page_no)) {

		logger::error["action"] << "setpage cannot switch to page " << page_no << ", page does not exist in layout" << std::endl;
		return;
	}

	if ( logger::loglevel() == logger::debug.id())
		logger::info["action"] << "setpage is switching to page " << LAYOUT::page_name(page_no) << std::endl;

	if ( !display -> setpage(page_no)) {

		logger::vverbose["action"] << "setpage action failed to switch to page " << page_no << std::endl;

	}
}
