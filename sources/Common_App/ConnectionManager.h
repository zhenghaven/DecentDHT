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
			void SetConfigManager(const Decent::Tools::ConfigManager& mgrRef);

			std::unique_ptr<Decent::Net::Connection> GetConnection2DecentDht(const Decent::Net::SmartMessages& hsMsg, uint32_t* outIpAddr = nullptr, uint16_t* outPort = nullptr);
			std::unique_ptr<Decent::Net::Connection> GetConnection2DecentDhtStore(const Decent::Net::SmartMessages& hsMsg, uint32_t* outIpAddr = nullptr, uint16_t* outPort = nullptr);
		}
	}
}
