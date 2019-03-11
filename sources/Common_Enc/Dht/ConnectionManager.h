#pragma once

#include <memory>

namespace Decent
{
	namespace Net
	{
		class EnclaveNetConnector;
	}
}

namespace Decent
{
	namespace Dht
	{
		namespace ConnectionManager
		{
			std::unique_ptr<Decent::Net::EnclaveNetConnector> GetConnection2DecentNode(uint64_t address);
			std::unique_ptr<Decent::Net::EnclaveNetConnector> GetConnection2DecentStore(uint64_t address);
		}
	}
}
