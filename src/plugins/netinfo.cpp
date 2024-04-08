#include <cstdint>

#include "logger.hpp"
#include "throws.hpp"
#include "plugin.hpp"
#include "netinfo.hpp"
#include "plugins/netinfo.hpp"

expr::VARIABLE plugin::NETINFO::fn_netinfo_exists(const expr::FUNCTION_ARGS& args) {

	std::string ifd;

	if ( args.empty()) {

		logger::warning["plugin"] << "netinfo requires 1 argument, interface name" << std::endl;
		return false;

	} else if ( ifd = args[0].string_convertible().empty() ? common::trim_ws(args[0].to_string()) : ""; ifd.empty()) {

		logger::warning["plugin"] << "syntax error with netinfo interface argument, argument not a string or is empty" << std::endl;
		return false;
	}

	std::map<std::string, netinfo::device> devices = netinfo::get_devices();
	return devices.contains(ifd);
}

expr::VARIABLE plugin::NETINFO::fn_netinfo_encap(const expr::FUNCTION_ARGS& args) {

	std::string ifd;

	if ( args.empty()) {

		logger::warning["plugin"] << "netinfo requires 1 argument, interface name" << std::endl;
		return "";

	} else if ( ifd = args[0].string_convertible().empty() ? common::trim_ws(args[0].to_string()) : ""; ifd.empty()) {

		logger::warning["plugin"] << "syntax error with netinfo interface argument, argument not a string or is empty" << std::endl;
		return "";
	}

	std::map<std::string, netinfo::device> devices = netinfo::get_devices();

	if ( !devices.contains(ifd)) {

		logger::warning["plugin"] << "netinfo cannot retrieve encapsulation for interface " << ifd << ", interface not found" << std::endl;
		return "";
	}

	return devices[ifd].encap;

}

expr::VARIABLE plugin::NETINFO::fn_netinfo_operstate(const expr::FUNCTION_ARGS& args) {

	std::string ifd;

	if ( args.empty()) {

		logger::warning["plugin"] << "netinfo requires 1 argument, interface name" << std::endl;
		return "";

	} else if ( ifd = args[0].string_convertible().empty() ? common::trim_ws(args[0].to_string()) : ""; ifd.empty()) {

		logger::warning["plugin"] << "syntax error with netinfo interface argument, argument not a string or is empty" << std::endl;
		return "";
	}

	std::map<std::string, netinfo::device> devices = netinfo::get_devices();

	if ( !devices.contains(ifd)) {

		logger::warning["plugin"] << "netinfo cannot retrieve operstate for interface " << ifd << ", interface not found" << std::endl;
		return "";
	}

	return devices[ifd].operstate;
}

expr::VARIABLE plugin::NETINFO::fn_netinfo_mtu(const expr::FUNCTION_ARGS& args) {

	std::string ifd;

	if ( args.empty()) {

		logger::warning["plugin"] << "netinfo requires 1 argument, interface name" << std::endl;
		return (double)0;

	} else if ( ifd = args[0].string_convertible().empty() ? common::trim_ws(args[0].to_string()) : ""; ifd.empty()) {

		logger::warning["plugin"] << "syntax error with netinfo interface argument, argument not a string or is empty" << std::endl;
		return (double)0;
	}

	std::map<std::string, netinfo::device> devices = netinfo::get_devices();

	if ( !devices.contains(ifd)) {

		logger::warning["plugin"] << "netinfo cannot retrieve mtu for interface " << ifd << ", interface not found" << std::endl;
		return (double)0;
	}

	return (double)devices[ifd].mtu;

}

expr::VARIABLE plugin::NETINFO::fn_netinfo_hwaddr(const expr::FUNCTION_ARGS& args) {

	std::string ifd;

	if ( args.empty()) {

		logger::warning["plugin"] << "netinfo requires 1 argument, interface name" << std::endl;
		return "";

	} else if ( ifd = args[0].string_convertible().empty() ? common::trim_ws(args[0].to_string()) : ""; ifd.empty()) {

		logger::warning["plugin"] << "syntax error with netinfo interface argument, argument not a string or is empty" << std::endl;
		return "";
	}

	std::map<std::string, netinfo::device> devices = netinfo::get_devices();

	if ( !devices.contains(ifd)) {

		logger::warning["plugin"] << "netinfo cannot retrieve hwaddr for interface " << ifd << ", interface not found" << std::endl;
		return "";
	}

	return devices[ifd].hwaddr;
}

expr::VARIABLE plugin::NETINFO::fn_netinfo_ip4addr(const expr::FUNCTION_ARGS& args) {

	std::string ifd;

	if ( args.empty()) {

		logger::warning["plugin"] << "netinfo requires 1 argument, interface name" << std::endl;
		return "";

	} else if ( ifd = args[0].string_convertible().empty() ? common::trim_ws(args[0].to_string()) : ""; ifd.empty()) {

		logger::warning["plugin"] << "syntax error with netinfo interface argument, argument not a string or is empty" << std::endl;
		return "";
	}

	std::map<std::string, netinfo::device> devices = netinfo::get_devices();

	if ( !devices.contains(ifd)) {

		logger::warning["plugin"] << "netinfo cannot retrieve ip address for interface " << ifd << ", interface not found" << std::endl;
		return "";
	}

	return devices[ifd].ipv4.empty() ? "" : devices[ifd].ipv4.front().addr;
}

expr::VARIABLE plugin::NETINFO::fn_netinfo_netmask(const expr::FUNCTION_ARGS& args) {

	std::string ifd;

	if ( args.empty()) {

		logger::warning["plugin"] << "netinfo requires 1 argument, interface name" << std::endl;
		return "";

	} else if ( ifd = args[0].string_convertible().empty() ? common::trim_ws(args[0].to_string()) : ""; ifd.empty()) {

		logger::warning["plugin"] << "syntax error with netinfo interface argument, argument not a string or is empty" << std::endl;
		return "";
	}

	std::map<std::string, netinfo::device> devices = netinfo::get_devices();

	if ( !devices.contains(ifd)) {

		logger::warning["plugin"] << "netinfo cannot retrieve netmaskn for interface " << ifd << ", interface not found" << std::endl;
		return "";
	}

	return devices[ifd].ipv4.empty() ? "" : devices[ifd].ipv4.front().netmask;
}

expr::VARIABLE plugin::NETINFO::fn_netinfo_cidrmask(const expr::FUNCTION_ARGS& args) {

	std::string ifd;

	if ( args.empty()) {

		logger::warning["plugin"] << "netinfo requires 1 argument, interface name" << std::endl;
		return "";

	} else if ( ifd = args[0].string_convertible().empty() ? common::trim_ws(args[0].to_string()) : ""; ifd.empty()) {

		logger::warning["plugin"] << "syntax error with netinfo interface argument, argument not a string or is empty" << std::endl;
		return "";
	}

	std::map<std::string, netinfo::device> devices = netinfo::get_devices();

	if ( !devices.contains(ifd)) {

		logger::warning["plugin"] << "netinfo cannot retrieve cidrmask for interface " << ifd << ", interface not found" << std::endl;
		return "";
	}

	return devices[ifd].ipv4.empty() ? "" : devices[ifd].ipv4.front().cidrmask;
}

expr::VARIABLE plugin::NETINFO::fn_netinfo_bcaddr(const expr::FUNCTION_ARGS& args) {

	std::string ifd;

	if ( args.empty()) {

		logger::warning["plugin"] << "netinfo requires 1 argument, interface name" << std::endl;
		return "";

	} else if ( ifd = args[0].string_convertible().empty() ? common::trim_ws(args[0].to_string()) : ""; ifd.empty()) {

		logger::warning["plugin"] << "syntax error with netinfo interface argument, argument not a string or is empty" << std::endl;
		return "";
	}

	std::map<std::string, netinfo::device> devices = netinfo::get_devices();

	if ( !devices.contains(ifd)) {

		logger::warning["plugin"] << "netinfo cannot retrieve broadcast address for interface " << ifd << ", interface not found" << std::endl;
		return "";
	}

	return devices[ifd].ipv4.empty() ? "" : devices[ifd].ipv4.front().broadcast;
}

expr::VARIABLE plugin::NETINFO::fn_netinfo_ip6addr(const expr::FUNCTION_ARGS& args) {

	std::string ifd;

	if ( args.empty()) {

		logger::warning["plugin"] << "netinfo requires 1 argument, interface name" << std::endl;
		return false;

	} else if ( ifd = args[0].string_convertible().empty() ? common::trim_ws(args[0].to_string()) : ""; ifd.empty()) {

		logger::warning["plugin"] << "syntax error with netinfo interface argument, argument not a string or is empty" << std::endl;
		return false;
	}

	std::map<std::string, netinfo::device> devices = netinfo::get_devices();

	if ( !devices.contains(ifd)) {

		logger::warning["plugin"] << "netinfo cannot retrieve ipv6 address for interface " << ifd << ", interface not found" << std::endl;
		return "";
	}

	return devices[ifd].ipv6.empty() ? "" : devices[ifd].ipv6.front().addr;
}

expr::VARIABLE plugin::NETINFO::fn_netinfo_prefix(const expr::FUNCTION_ARGS& args) {

	std::string ifd;

	if ( args.empty()) {

		logger::warning["plugin"] << "netinfo requires 1 argument, interface name" << std::endl;
		return "";

	} else if ( ifd = args[0].string_convertible().empty() ? common::trim_ws(args[0].to_string()) : ""; ifd.empty()) {

		logger::warning["plugin"] << "syntax error with netinfo interface argument, argument not a string or is empty" << std::endl;
		return "";
	}

	std::map<std::string, netinfo::device> devices = netinfo::get_devices();

	if ( !devices.contains(ifd)) {

		logger::warning["plugin"] << "netinfo cannot retrieve ipv6 prefix for interface " << ifd << ", interface not found" << std::endl;
		return "";
	}

	return devices[ifd].ipv6.empty() ? "" : devices[ifd].ipv6.front().prefix;
}

expr::VARIABLE plugin::NETINFO::fn_netinfo_scope(const expr::FUNCTION_ARGS& args) {

	std::string ifd;

	if ( args.empty()) {

		logger::warning["plugin"] << "netinfo requires 1 argument, interface name" << std::endl;
		return "";

	} else if ( ifd = args[0].string_convertible().empty() ? common::trim_ws(args[0].to_string()) : ""; ifd.empty()) {

		logger::warning["plugin"] << "syntax error with netinfo interface argument, argument not a string or is empty" << std::endl;
		return "";
	}

	std::map<std::string, netinfo::device> devices = netinfo::get_devices();

	if ( !devices.contains(ifd)) {

		logger::warning["plugin"] << "netinfo cannot retrieve ipv6 scope for interface " << ifd << ", interface not found" << std::endl;
		return "";
	}

	return devices[ifd].ipv6.empty() ? "" : devices[ifd].ipv6.front().scope;
}

expr::VARIABLE plugin::NETINFO::fn_netinfo_rx_packets(const expr::FUNCTION_ARGS& args) {

	std::string ifd;

	if ( args.empty()) {

		logger::warning["plugin"] << "netinfo requires 1 argument, interface name" << std::endl;
		return "";

	} else if ( ifd = args[0].string_convertible().empty() ? common::trim_ws(args[0].to_string()) : ""; ifd.empty()) {

		logger::warning["plugin"] << "syntax error with netinfo interface argument, argument not a string or is empty" << std::endl;
		return "";
	}

	std::map<std::string, netinfo::device> devices = netinfo::get_devices();

	if ( !devices.contains(ifd)) {

		logger::warning["plugin"] << "netinfo cannot retrieve rx packets for interface " << ifd << ", interface not found" << std::endl;
		return "";
	}

	return std::to_string(devices[ifd].rx.packets);
}

expr::VARIABLE plugin::NETINFO::fn_netinfo_rx_bytes(const expr::FUNCTION_ARGS& args) {

	std::string ifd;
	std::string format = "auto";

	if ( args.empty()) {

		logger::warning["plugin"] << "netinfo requires 1 argument, interface name" << std::endl;
		return "";

	} else if ( ifd = args[0].string_convertible().empty() ? common::trim_ws(args[0].to_string()) : ""; ifd.empty()) {

		logger::warning["plugin"] << "syntax error with netinfo interface argument, argument not a string or is empty" << std::endl;
		return "";
	}

	if ( args.size() > 1 && args[1].string_convertible().empty()) {

		format = common::to_lower(args[1]);

		if ( format.empty()) {

			logger::warning["plugin"] << "netinfo cannot provide stats in selected format, format empty or not a string" << std::endl;
			format = "auto";

		} else if ( !common::is_any_of(format, { "auto", "b", "bytes", "kb", "kib", "mb", "mib", "gb", "gib" })) {

			logger::warning["plugin"] << "netinfo cannot use '" << format << "' as format, unknown type" << std::endl;
			logger::verbose["plugin"] << "available formats for netinfo are auto, bytes, kib, mib and gib" << std::endl;
			format = "auto";
		}
	}

	std::map<std::string, netinfo::device> devices = netinfo::get_devices();

	if ( !devices.contains(ifd)) {

		logger::warning["plugin"] << "netinfo cannot retrieve rx bytes for interface " << ifd << ", interface not found" << std::endl;
		return "";
	}

	std::string ret;

	if ( format == "b" || format == "bytes" )
		ret = std::to_string(devices[ifd].rx.bytes);
	else if ( format == "kib" || format == "kb" )
		ret = common::to_string(devices[ifd].rx.KiB());
	else if ( format == "mib" || format == "mb" )
		ret = common::to_string(devices[ifd].rx.MiB());
	else if ( format == "gib" || format == "gb" )
		ret = common::to_string(devices[ifd].rx.GiB());
	else {

		std::string suffix = "GiB";
		double v = devices[ifd].rx.GiB();

		if ( v == 0 ) {
			suffix = "MiB";
			v = devices[ifd].rx.MiB();
		}

		if ( v == 0 ) {
			suffix = "KiB";
			v = devices[ifd].rx.KiB();
		}

		if ( v == 0 )
			return std::to_string(devices[ifd].rx.bytes) + " bytes";
		else ret = common::to_string(v) + " " + suffix;
	}

	return ret;
}

expr::VARIABLE plugin::NETINFO::fn_netinfo_tx_packets(const expr::FUNCTION_ARGS& args) {

	std::string ifd;

	if ( args.empty()) {

		logger::warning["plugin"] << "netinfo requires 1 argument, interface name" << std::endl;
		return "";

	} else if ( ifd = args[0].string_convertible().empty() ? common::trim_ws(args[0].to_string()) : ""; ifd.empty()) {

		logger::warning["plugin"] << "syntax error with netinfo interface argument, argument not a string or is empty" << std::endl;
		return "";
	}

	std::map<std::string, netinfo::device> devices = netinfo::get_devices();

	if ( !devices.contains(ifd)) {

		logger::warning["plugin"] << "netinfo cannot retrieve tx packets for interface " << ifd << ", interface not found" << std::endl;
		return "";
	}

	return std::to_string(devices[ifd].tx.packets);
}

expr::VARIABLE plugin::NETINFO::fn_netinfo_tx_bytes(const expr::FUNCTION_ARGS& args) {

	std::string ifd;
	std::string format = "auto";

	if ( args.empty()) {

		logger::warning["plugin"] << "netinfo requires 1 argument, interface name" << std::endl;
		return "";

	} else if ( ifd = args[0].string_convertible().empty() ? common::trim_ws(args[0].to_string()) : ""; ifd.empty()) {

		logger::warning["plugin"] << "syntax error with netinfo interface argument, argument not a string or is empty" << std::endl;
		return "";
	}

	if ( args.size() > 1 && args[1].string_convertible().empty()) {

		format = common::to_lower(args[1]);

		if ( format.empty()) {

			logger::warning["plugin"] << "netinfo cannot provide stats in selected format, format empty or not a string" << std::endl;
			format = "auto";

		} else if ( !common::is_any_of(format, { "auto", "b", "bytes", "kb", "kib", "mb", "mib", "gb", "gib" })) {

			logger::warning["plugin"] << "netinfo cannot use '" << format << "' as format, unknown type" << std::endl;
			logger::verbose["plugin"] << "available formats for netinfo are auto, bytes, kib, mib and gib" << std::endl;
			format = "auto";
		}
	}

	std::map<std::string, netinfo::device> devices = netinfo::get_devices();

	if ( !devices.contains(ifd)) {

		logger::warning["plugin"] << "netinfo cannot retrieve tx bytes for interface " << ifd << ", interface not found" << std::endl;
		return "";
	}

	std::string ret;

	if ( format == "b" || format == "bytes" )
		ret = std::to_string(devices[ifd].rx.bytes);
	else if ( format == "kib" || format == "kb" )
		ret = common::to_string(devices[ifd].rx.KiB());
	else if ( format == "mib" || format == "mb" )
		ret = common::to_string(devices[ifd].rx.MiB());
	else if ( format == "gib" || format == "gb" )
		ret = common::to_string(devices[ifd].rx.GiB());
	else {

		std::string suffix = "GiB";
		double v = devices[ifd].rx.GiB();

		if ( v == 0 ) {
			suffix = "MiB";
			v = devices[ifd].rx.MiB();
		}

		if ( v == 0 ) {
			suffix = "KiB";
			v = devices[ifd].rx.KiB();
		}

		if ( v == 0 )
			return std::to_string(devices[ifd].rx.bytes) + " bytes";
		else ret = common::to_string(v) + " " + suffix;
	}

	return ret;
}

plugin::NETINFO::NETINFO(CONFIG::MAP *cfg) {

	logger::vverbose["plugin"] << "initializing plugin netinfo" << std::endl;

	CONFIG::functions.append({ "netinfo::exists", plugin::NETINFO::fn_netinfo_exists });
	CONFIG::functions.append({ "netinfo::encap", plugin::NETINFO::fn_netinfo_encap });
	CONFIG::functions.append({ "netinfo::operstate", plugin::NETINFO::fn_netinfo_operstate });
	CONFIG::functions.append({ "netinfo::mtu", plugin::NETINFO::fn_netinfo_mtu });
	CONFIG::functions.append({ "netinfo::hwaddr", plugin::NETINFO::fn_netinfo_hwaddr });
	CONFIG::functions.append({ "netinfo::ip4addr", plugin::NETINFO::fn_netinfo_ip4addr });
	CONFIG::functions.append({ "netinfo::netmask", plugin::NETINFO::fn_netinfo_netmask });
	CONFIG::functions.append({ "netinfo::cidrmask", plugin::NETINFO::fn_netinfo_cidrmask });
	CONFIG::functions.append({ "netinfo::bcaddr", plugin::NETINFO::fn_netinfo_bcaddr });
	CONFIG::functions.append({ "netinfo::ip6addr", plugin::NETINFO::fn_netinfo_ip6addr });
	CONFIG::functions.append({ "netinfo::prefix", plugin::NETINFO::fn_netinfo_prefix });
	CONFIG::functions.append({ "netinfo::scope", plugin::NETINFO::fn_netinfo_scope });
	CONFIG::functions.append({ "netinfo::rx::packets", plugin::NETINFO::fn_netinfo_rx_packets });
	CONFIG::functions.append({ "netinfo::rx::bytes", plugin::NETINFO::fn_netinfo_rx_bytes });
	CONFIG::functions.append({ "netinfo::tx:packets", plugin::NETINFO::fn_netinfo_tx_packets });
	CONFIG::functions.append({ "netinfo::tx::bytes", plugin::NETINFO::fn_netinfo_tx_bytes });
}

plugin::NETINFO::~NETINFO() {

	CONFIG::functions.erase("netinfo::exists");
	CONFIG::functions.erase("netinfo::encap");
	CONFIG::functions.erase("netinfo::operstate");
	CONFIG::functions.erase("netinfo::mtu");
	CONFIG::functions.erase("netinfo::hwaddr");
	CONFIG::functions.erase("netinfo::ip4addr");
	CONFIG::functions.erase("netinfo::netmask");
	CONFIG::functions.erase("netinfo::cidrmask");
	CONFIG::functions.erase("netinfo::bcaddr");
	CONFIG::functions.erase("netinfo::ip6addr");
	CONFIG::functions.erase("netinfo::prefix");
	CONFIG::functions.erase("netinfo::scope");
	CONFIG::functions.erase("netinfo::rx::packets");
	CONFIG::functions.erase("netinfo::rx::bytes");
	CONFIG::functions.erase("netinfo::tx:packets");
	CONFIG::functions.erase("netinfo::tx::bytes");

}
