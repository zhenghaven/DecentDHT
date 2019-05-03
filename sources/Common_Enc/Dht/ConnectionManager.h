#pragma once

#include <memory>

namespace Decent
{
	namespace Net
	{
		class ConnectionBase;
	}
}

namespace Decent
{
	namespace Dht
	{
		namespace ConnectionManager
		{
			std::unique_ptr<Decent::Net::ConnectionBase> GetConnection2DecentNode(uint64_t address);
			std::unique_ptr<Decent::Net::ConnectionBase> GetConnection2DecentStore(uint64_t address);
		}
	}
}
