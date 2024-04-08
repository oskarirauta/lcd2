#pragma once

#include <string>
#include <vector>
#include <map>

#include "common.hpp"
#include "config.hpp"
#include "properties.hpp"
#include "lowercase_map.hpp"
#include "expr/expression.hpp"

class plugin {

	public:
		class EXEC;
		class CPUINFO;
		class MEMINFO;
		class NETINFO;
		class FILE;
		class TEST;
		class UNAME;
		class FS;
		class UPTIME;

		class PLUGIN : public PROPERTIES {

			protected:

				bool _enabled = true;

			public:

				virtual const std::string type() const = 0;
				virtual bool enabled();
				virtual int interval();
				virtual bool update();

				PLUGIN();
				virtual ~PLUGIN() = 0;
		};

		// type name, configurable
		static common::lowercase_map<bool> types;

		common::lowercase_map<std::shared_ptr<plugin::PLUGIN>> plugins;

		common::lowercase_map<std::shared_ptr<plugin::PLUGIN>>::iterator begin();
		common::lowercase_map<std::shared_ptr<plugin::PLUGIN>>::iterator end();
		common::lowercase_map<std::shared_ptr<plugin::PLUGIN>>::size_type size();
                common::lowercase_map<std::shared_ptr<plugin::PLUGIN>>::const_iterator begin() const;
                common::lowercase_map<std::shared_ptr<plugin::PLUGIN>>::const_iterator end() const;
                common::lowercase_map<std::shared_ptr<plugin::PLUGIN>>::size_type size() const;

		bool contains(const std::string& name);
		void erase(const std::string& name);
		bool empty();

		bool is_configurable(const std::string& name);
		void add(const std::string& name, CONFIG::MAP *cfg);

		plugin::PLUGIN* operator[](const std::string& name);

		plugin();
		~plugin();
};
