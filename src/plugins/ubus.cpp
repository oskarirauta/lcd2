#ifdef WITH_UBUS

#include <string>
#include <mutex>
#include <chrono>
#include <unordered_map>
#include <cstdlib>

extern "C" {
#include <libubus.h>
#include <libubox/blobmsg_json.h>
}

#include "logger.hpp"
#include "json/json.hpp"
#include "json/json_error.hpp"
#include "plugins/ubus.hpp"

struct ubus_call_result {
	std::string json_str;
	bool ok = false;
};

static void ubus_response_cb(struct ubus_request *req, int, struct blob_attr *msg) {
	auto *r = static_cast<ubus_call_result*>(req->priv);
	if (!msg) return;
	char *s = blobmsg_format_json(msg, true);
	if (s) {
		r->json_str = s;
		r->ok = true;
		free(s);
	}
}

static std::string g_socket_path = "/var/run/ubus/ubus.sock";

namespace {

struct cache_entry {
	JSON data;
	std::chrono::steady_clock::time_point ts;
};

std::mutex cache_mtx;
std::unordered_map<std::string, cache_entry> result_cache;
constexpr long CACHE_TTL_MS = 2000;

JSON resolve_path(JSON j, const std::string& path) {

	if (path.empty()) return j;

	JSON *cur = &j;
	std::string rem = path;

	while (!rem.empty()) {

		size_t dot = rem.find('.');
		std::string seg = (dot == std::string::npos) ? rem : rem.substr(0, dot);
		rem = (dot == std::string::npos) ? "" : rem.substr(dot + 1);

		if (*cur == JSON::OBJECT) {

			if (!cur->contains(seg)) return nullptr;
			cur = &(*cur)[seg];

		} else if (*cur == JSON::ARRAY) {

			try {
				size_t idx = std::stoul(seg);
				if (idx >= cur->size()) return nullptr;
				cur = &(*cur)[idx];
			} catch (...) { return nullptr; }

		} else return nullptr;
	}

	return *cur;
}

JSON ubus_fetch(const std::string& path, const std::string& method, const std::string& args_json) {

	std::string key = path + "|" + method + "|" + args_json;

	{
		std::lock_guard<std::mutex> lk(cache_mtx);
		auto it = result_cache.find(key);
		if (it != result_cache.end()) {
			auto age = std::chrono::duration_cast<std::chrono::milliseconds>(
				std::chrono::steady_clock::now() - it->second.ts).count();
			if (age < CACHE_TTL_MS)
				return it->second.data;
		}
	}

	struct ubus_context *ctx = ubus_connect(g_socket_path.c_str());
	if (!ctx) {
		logger::error["plugin"] << "ubus: cannot connect to ubusd" << std::endl;
		return nullptr;
	}

	uint32_t id;
	if (ubus_lookup_id(ctx, path.c_str(), &id) != UBUS_STATUS_OK) {
		logger::error["plugin"] << "ubus: object '" << path << "' not found" << std::endl;
		ubus_free(ctx);
		return nullptr;
	}

	struct blob_buf b = {};
	blob_buf_init(&b, 0);

	if (!args_json.empty() && args_json != "{}") {
		if (!blobmsg_add_json_from_string(&b, args_json.c_str())) {
			logger::warning["plugin"] << "ubus: failed to parse args, using empty args" << std::endl;
			blob_buf_free(&b);
			blob_buf_init(&b, 0);
		}
	}

	ubus_call_result res;
	int ret = ubus_invoke(ctx, id, method.c_str(), b.head, ubus_response_cb, &res, 3000);

	blob_buf_free(&b);
	ubus_free(ctx);

	if (ret != UBUS_STATUS_OK || !res.ok) {
		logger::error["plugin"] << "ubus: " << path << " " << method
			<< " failed: " << ubus_strerror(ret) << std::endl;
		return nullptr;
	}

	JSON result;
	try {
		result = JSON::parse(res.json_str);
	} catch (const JSON::exception& e) {
		logger::error["plugin"] << "ubus: JSON parse error: " << e.what() << std::endl;
		return nullptr;
	}

	std::lock_guard<std::mutex> lk(cache_mtx);
	result_cache[key] = { result, std::chrono::steady_clock::now() };
	return result;
}

} // namespace

expr::VARIABLE plugin::UBUS::fn_ubus(const expr::FUNCTION_ARGS& args) {

	if (args.size() < 2) {
		logger::error["plugin"] << "ubus: requires at least 2 arguments: path, method" << std::endl;
		return "";
	}

	auto str = [](const expr::VARIABLE& v) -> std::string {
		return v.string_convertible().empty() ? v.to_string() : "";
	};

	std::string path   = str(args[0]);
	std::string method = str(args[1]);
	std::string jpath  = (args.size() >= 3) ? str(args[2]) : "";
	std::string iargs  = (args.size() >= 4) ? str(args[3]) : "";

	if (path.empty() || method.empty()) {
		logger::error["plugin"] << "ubus: path and method must be non-empty strings" << std::endl;
		return "";
	}

	JSON root = ubus_fetch(path, method, iargs);
	if (root == JSON::TYPE::NULLPTR) return "";

	JSON val = resolve_path(root, jpath);

	switch (val.type()) {
		case JSON::INT:    return static_cast<double>(static_cast<long long>(val));
		case JSON::FLOAT:  return static_cast<double>(static_cast<long double>(val));
		case JSON::BOOL:   return static_cast<bool>(val) ? 1.0 : 0.0;
		case JSON::STRING: return val.to_string();
		default:           return val.dump(false);
	}
}

void plugin::UBUS::configure(CONFIG::MAP *cfg) {

	if (!cfg) return;

	for (auto& [k, v] : *cfg) {
		if (k == "socket" && std::holds_alternative<std::string>(v)) {
			g_socket_path = std::get<std::string>(v);
			logger::verbose["plugin"] << "ubus: socket path set to " << g_socket_path << std::endl;
		} else if (k != "class" && k != "type") {
			logger::warning["plugin"] << "ubus: unknown option '" << k << "', ignored" << std::endl;
		}
	}
}

plugin::UBUS::UBUS(CONFIG::MAP *cfg) {
	logger::vverbose["plugin"] << "plugin ubus initialized" << std::endl;
	plugin::UBUS::configure(cfg);
	CONFIG::functions.append({ "ubus", plugin::UBUS::fn_ubus });
}

plugin::UBUS::~UBUS() {
	CONFIG::functions.erase("ubus");
}

#endif // WITH_UBUS
