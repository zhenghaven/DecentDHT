#pragma once

#include <memory>

namespace Decent
{
	namespace Net
	{
		class Connection;
		class SmartMessages;
	}

	namespace Tools
	{
		class ConfigManager;
	}
}

namespace Decent
{
	namespace Dht
	{
		namespace ConnectionManager
		{
			std::unique_ptr<Decent::Net::Connection> GetConnection2DecentDht(const Decent::Net::SmartMessages& hsMsg, uint64_t address);
			std::unique_ptr<Decent::Net::Connection> GetConnection2DecentDhtStore(const Decent::Net::SmartMessages& hsMsg, uint64_t address);
		}
	}
}
