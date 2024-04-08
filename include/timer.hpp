#pragma once

#include <string>
#include <mutex>

#include "properties.hpp"

class DISPLAY;
class SCHEDULER;
class LAYOUT;

class TIMER : public PROPERTIES {

	friend class DISPLAY;
	friend class SCHEDULER;
	friend class LAYOUT;
	friend class LAYOUT::PAGE;

	private:
		const std::string new_expression_name() const;
		void evaluate(const std::string& expr);

	protected:

		std::string _name;

		std::chrono::milliseconds last_updated = std::chrono::milliseconds(0);
		std::mutex _m;

	public:

		virtual const std::string type() const;
		virtual const std::string name() const;

		const std::string dump();
		const std::vector<std::string> expressions() const;
		int interval();
		bool active();
		const std::string get_action() const;

		const bool is_global(LAYOUT* layout) const;
		const bool on_page(LAYOUT* layout, int page_no) const;

		virtual bool update();

		TIMER();
		TIMER(const std::string& name, CONFIG::MAP *cfg);
		~TIMER() {};

		TIMER& operator =(const TIMER& other);

		friend std::ostream& operator <<(std::ostream& os, TIMER const& t);
		friend std::ostream& operator <<(std::ostream& os, TIMER const *t);

};

std::ostream& operator <<(std::ostream& os, TIMER const& t);
std::ostream& operator <<(std::ostream& os, TIMER const *t);
