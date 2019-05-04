#include "DhtConnectionPool.h"

#include <DecentApi/Common/make_unique.h>
#include <DecentApi/Common/Ra/States.h>
#include <DecentApi/Common/Net/ConnectionBase.h>
#include <DecentApi/Common/Net/TlsCommLayer.h>
#include <DecentApi/Common/Tools/SharedCachingQueue.h>
#include <DecentApi/Common/MbedTls/Session.h>

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

	static Tools::SharedCachingQueue<uint64_t, MbedTlsObj::Session>& GetSessionCache()
	{
		static Tools::SharedCachingQueue<uint64_t, MbedTlsObj::Session> inst(10);
		return inst;
	}
}

CntPair DhtConnectionPool::GetNew(const uint64_t & addr, Ra::States & state)
{
	std::unique_ptr<ConnectionBase> connection = ConnectionManager::GetConnection2DecentNode(addr);

	std::shared_ptr<MbedTlsObj::Session> session = GetSessionCache().Get(addr);
	
	std::unique_ptr<TlsCommLayer> tls = Tools::make_unique<TlsCommLayer>(*connection, GetClientTlsConfigDhtNode(state), true, session);

	if (!session)
	{
		GetSessionCache().Put(addr, tls->GetSessionCopy(), false);
	}

	std::unique_ptr<SecureCommLayer> comm = std::move(tls);
	return CntPair(connection, comm);
}
