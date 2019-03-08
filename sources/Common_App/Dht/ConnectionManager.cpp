#include "ConnectionManager.h"

#include <boost/asio/ip/address_v4.hpp>

#include <DecentApi/Common/Common.h>
#include <DecentApi/CommonApp/Net/TCPConnection.h>
#include <DecentApi/CommonApp/Tools/ConfigManager.h>

#include "../../Common/Dht/AppNames.h"
#include "Messages.h"

using namespace Decent::Tools;
using namespace Decent::Net;
using namespace Decent::Dht;

namespace
{

	static inline std::unique_ptr<Connection> InternalGetConnection(const SmartMessages& hsMsg, uint64_t address)
	{
		try
		{
			std::unique_ptr<Connection> connection = std::make_unique<TCPConnection>(address);

			if (connection)
			{
				connection->SendPack(hsMsg);
			}
			return std::move(connection);
		}
		catch (const std::exception& e)
		{
			const char* msgStr = e.what();
			LOGW("Failed to establish connection. (Err Msg: %s)", msgStr);
			return nullptr;
		}
	}
}


std::unique_ptr<Connection> ConnectionManager::GetConnection2DecentDht(const SmartMessages& hsMsg, uint64_t address)
{
	return InternalGetConnection(hsMsg, address);
}

extern "C" void ocall_decent_dht_cnt_mgr_close_cnt(void* cnt_ptr)
{
	delete static_cast<Connection*>(cnt_ptr);
}

extern "C" void ocall_decent_dht_cnt_mgr_get_dht(void** out_cnt_ptr, uint64_t address)
{
	if (!out_cnt_ptr)
	{
		return;
	}
	std::unique_ptr<Connection> cnt = ConnectionManager::GetConnection2DecentDht(FromDht(), address);

	*out_cnt_ptr = cnt.release();
}


