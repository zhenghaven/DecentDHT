#include "DhtConnectionPool.h"

#include <DecentApi/Common/make_unique.h>
#include <DecentApi/Common/Ra/States.h>
#include <DecentApi/Common/Net/ConnectionBase.h>
#include <DecentApi/Common/Net/TlsCommLayer.h>

#include <DecentApi/CommonEnclave/Ra/TlsConfigSameEnclave.h>

#include "ConnectionManager.h"

using namespace Decent;
using namespace Decent::Net;
using namespace Decent::Dht;

namespace
{
	static std::shared_ptr<Ra::TlsConfigSameEnclave> GetClientTlsConfigDhtNode(Ra::States& state)
	{
		static std::shared_ptr<Ra::TlsConfigSameEnclave> tlsCfg = std::make_shared<Ra::TlsConfigSameEnclave>(state, Ra::TlsConfig::Mode::ClientHasCert, nullptr);
		return tlsCfg;
	}
}

CntPair DhtConnectionPool::GetNew(const uint64_t & addr, Ra::States & state)
{
	std::unique_ptr<ConnectionBase> connection = ConnectionManager::GetConnection2DecentNode(addr);
	std::unique_ptr<SecureCommLayer> tls = Tools::make_unique<TlsCommLayer>(*connection, GetClientTlsConfigDhtNode(state), true);
	return CntPair(connection, tls);
}
