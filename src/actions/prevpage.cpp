#include "common.hpp"
#include "logger.hpp"
#include "display.hpp"
#include "layout.hpp"
#include "actions/prevpage.hpp"

void action::PREVPAGE::execute(const std::vector<expr::RESULT>& args) {

	if ( !args.empty()) {

		logger::warning["action"] << "arguments are not used with page::next action, arguments ignored" << std::endl;

	} else if ( display -> layout -> page_sequence.empty()) {

		logger::error["action"] << "action page::next is not available, layout page sequence is empty" << std::endl;
		return;
	}

	if ( display -> layout -> prev_page_index < 0 )
		display -> layout -> prev_page_index = display -> layout -> page_sequence.size() - 1;

	int page_no = display -> layout -> page_sequence[display -> layout -> prev_page_index];

	if ( !display -> layout -> pages.contains(page_no)) {

		logger::error["action"] << "page::next cannot switch to page " << page_no << ", page does not exist in layout" << std::endl;

	} else {

		if ( logger::loglevel() == logger::debug.id())
			logger::info["action"] << "page::next is switching to page " << LAYOUT::page_name(page_no) << std::endl;

		if ( !display -> setpage(page_no)) {

			logger::vverbose["action"] << "page::next action failed to switch to page " << page_no << std::endl;
		}
	}

	display -> layout -> next_page_index = display -> layout -> prev_page_index;
	display -> layout -> prev_page_index--;
}
