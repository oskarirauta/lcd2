#pragma once

#include <chrono>

#include "common.hpp"
#include "lowercase_map.hpp"
#include "config.hpp"
#include "expr/expression.hpp"
#include "plugin.hpp"

/* available attributes:
 - vendor
 - family
 - model
 - mhz
 - cache
 - cores
 - cpuid
 - clflush
 - stepping
 - microcode
 - fpu
 - wp
 - bogomips
 - cache_alignment
 - load

load is cpu's load in percent. If /proc/cpuinfo did not
contain info about these values, empty "" will be returned
*/

class plugin::CPUINFO : public plugin::PLUGIN {

	protected:

		std::chrono::milliseconds last_updated = std::chrono::milliseconds(0);

	public:

		virtual const std::string type() const override { return "cpuinfo"; }
		virtual bool update() override;
		virtual int interval() override;

		explicit CPUINFO(CONFIG::MAP *cfg);
		~CPUINFO();
};
