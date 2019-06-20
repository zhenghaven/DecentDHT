#include "DhtSecureConnectionMgr.h"

#include <DecentApi/Common/make_unique.h>
#include <DecentApi/Common/Net/ConnectionBase.h>
#include <DecentApi/Common/Net/TlsCommLayer.h>
#include <DecentApi/Common/Tools/SharedCachingQueue.h>
#include <DecentApi/Common/MbedTls/Session.h>

#include <DecentApi/CommonEnclave/Ra/TlsConfigSameEnclave.h>

#include "ConnectionManager.h"
#include "DhtStates.h"

using namespace Decent;
using namespace Decent::Net;
using namespace Decent::Dht;

namespace
{
	static std::shared_ptr<Ra::TlsConfigSameEnclave> GetClientTlsConfigDhtNode(DhtStates& state)
	{
		static std::shared_ptr<Ra::TlsConfigSameEnclave> tlsCfg = std::make_shared<Ra::TlsConfigSameEnclave>(state, Ra::TlsConfig::Mode::ClientHasCert, nullptr);
		return tlsCfg;
	}
}

DhtSecureConnectionMgr::DhtSecureConnectionMgr(size_t maxOutCnt) :
	m_sessionCache(maxOutCnt)
{}

DhtSecureConnectionMgr::~DhtSecureConnectionMgr()
{
}

CntPair DhtSecureConnectionMgr::GetNew(const uint64_t& addr, DhtStates& state)
{
	std::unique_ptr<ConnectionBase> connection = ConnectionManager::GetConnection2DecentNode(addr);

	std::shared_ptr<MbedTlsObj::Session> session = m_sessionCache.Get(addr);
	
	std::unique_ptr<TlsCommLayer> tls = Tools::make_unique<TlsCommLayer>(*connection, GetClientTlsConfigDhtNode(state), true, session);

	if (!session)
	{
		m_sessionCache.Put(addr, tls->GetSessionCopy(), false);
	}

	std::unique_ptr<SecureCommLayer> comm = std::move(tls);
	return CntPair(connection, comm);
}
