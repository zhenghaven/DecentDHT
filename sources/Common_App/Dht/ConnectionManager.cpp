#include "ConnectionManager.h"

#include <boost/asio/ip/address_v4.hpp>

#include <DecentApi/Common/Common.h>
#include <DecentApi/CommonApp/Net/TCPConnection.h>
#include <DecentApi/CommonApp/Tools/ConfigManager.h>

#include "../../Common/Dht/AppNames.h"

using namespace Decent::Tools;
using namespace Decent::Net;
using namespace Decent::Dht;

namespace
{
	static const ConfigManager* gsk_configMgrPtr = nullptr;

	static inline std::unique_ptr<Connection> InternalGetConnection(const ConfigItem& configItem, const SmartMessages& hsMsg, uint32_t* outIpAddr, uint16_t* outPort)
	{
		uint32_t ip = boost::asio::ip::address_v4::from_string(configItem.GetAddr()).to_uint();
		if (outIpAddr)
		{
			*outIpAddr = ip;
		}
		if (outPort)
		{
			*outPort = configItem.GetPort();
		}

		try
		{
			std::unique_ptr<Connection> connection = std::make_unique<TCPConnection>(ip, configItem.GetPort());

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

void ConnectionManager::SetConfigManager(const ConfigManager & mgrRef)
{
	gsk_configMgrPtr = &mgrRef;
}

std::unique_ptr<Connection> ConnectionManager::GetConnection2DecentDht(const SmartMessages& hsMsg, uint32_t* outIpAddr, uint16_t* outPort)
{
	const ConfigItem& pasMgmItem = gsk_configMgrPtr->GetItem(AppNames::sk_decentDHT);

	return InternalGetConnection(pasMgmItem, hsMsg, outIpAddr, outPort);
}

extern "C" void ocall_decent_dht_cnt_mgr_close_cnt(void* cnt_ptr)
{
	delete static_cast<Connection*>(cnt_ptr);
}
