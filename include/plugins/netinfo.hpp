#pragma once

#include <mutex>
#include <thread>
#include "common.hpp"
#include "lowercase_map.hpp"
#include "config.hpp"
#include "expr/expression.hpp"
#include "plugin.hpp"

class plugin::NETINFO : public plugin::PLUGIN {

	public:

		virtual const std::string type() const override { return "netinfo"; }

		explicit NETINFO(CONFIG::MAP *cfg);
		~NETINFO();

	static expr::VARIABLE fn_netinfo_exists(const expr::FUNCTION_ARGS& args);
	static expr::VARIABLE fn_netinfo_encap(const expr::FUNCTION_ARGS& args);
	static expr::VARIABLE fn_netinfo_operstate(const expr::FUNCTION_ARGS& args);
	static expr::VARIABLE fn_netinfo_mtu(const expr::FUNCTION_ARGS& args);
	static expr::VARIABLE fn_netinfo_hwaddr(const expr::FUNCTION_ARGS& args);
	static expr::VARIABLE fn_netinfo_ip4addr(const expr::FUNCTION_ARGS& args);
	static expr::VARIABLE fn_netinfo_netmask(const expr::FUNCTION_ARGS& args);
	static expr::VARIABLE fn_netinfo_cidrmask(const expr::FUNCTION_ARGS& args);
	static expr::VARIABLE fn_netinfo_bcaddr(const expr::FUNCTION_ARGS& args);
	static expr::VARIABLE fn_netinfo_ip6addr(const expr::FUNCTION_ARGS& args);
	static expr::VARIABLE fn_netinfo_prefix(const expr::FUNCTION_ARGS& args);
	static expr::VARIABLE fn_netinfo_scope(const expr::FUNCTION_ARGS& args);
	static expr::VARIABLE fn_netinfo_rx_packets(const expr::FUNCTION_ARGS& args);
	static expr::VARIABLE fn_netinfo_rx_bytes(const expr::FUNCTION_ARGS& args);
	static expr::VARIABLE fn_netinfo_tx_packets(const expr::FUNCTION_ARGS& args);
	static expr::VARIABLE fn_netinfo_tx_bytes(const expr::FUNCTION_ARGS& args);

};
