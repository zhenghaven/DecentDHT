#include "DhtSecureConnectionMgr.h"

#include <DecentApi/Common/make_unique.h>
#include <DecentApi/Common/Net/ConnectionBase.h>
#include <DecentApi/Common/Net/TlsCommLayer.h>
#include <DecentApi/Common/Tools/SharedCachingQueue.h>
#include <DecentApi/Common/MbedTls/Session.h>

#include <DecentApi/CommonEnclave/Ra/TlsConfigSameEnclave.h>

#include "ConnectionManager.h"
#include "DhtStates.h"

#ifdef DECENT_DHT_NAIVE_RA_VER
#include <DecentApi/Common/Ra/KeyContainer.h>
#include <DecentApi/Common/SGX/RaProcessorSp.h>
#include <DecentApi/CommonEnclave/SGX/RaProcessorClient.h>
#include <DecentApi/CommonEnclave/SGX/RaMutualCommLayer.h>
#endif // DECENT_DHT_NAIVE_RA_VER

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

#ifdef DECENT_DHT_NAIVE_RA_VER
	static Sgx::RaProcessorSp::SgxQuoteVerifier quoteVrfy = [](const sgx_quote_t&)
	{
		return true;
	};
#endif // DECENT_DHT_NAIVE_RA_VER
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

	auto session = m_sessionCache.Get(addr);

#ifdef DECENT_DHT_NAIVE_RA_VER

	std::unique_ptr<Sgx::RaMutualCommLayer> tmpSecComm =
		Tools::make_unique<Sgx::RaMutualCommLayer>(*connection,
			Tools::make_unique<Sgx::RaProcessorClient>(state.GetEnclaveId(),
				Sgx::RaProcessorClient::sk_acceptAnyPubKey, Sgx::RaProcessorClient::sk_acceptAnyRaConfig),
			Tools::make_unique<Sgx::RaProcessorSp>(state.GetIasConnector(),
				state.GetKeyContainer().GetSignKeyPair(), state.GetSpid(),
				Sgx::RaProcessorSp::sk_defaultRpDataVrfy, quoteVrfy),
			session);

	std::shared_ptr<const Sgx::RaClientSession> neSession = tmpSecComm->GetClientSession();
	if (session != neSession)
	{
		m_sessionCache.Put(addr, neSession, false);
	}

#else

	std::unique_ptr<TlsCommLayer> tmpSecComm = Tools::make_unique<TlsCommLayer>(*connection, GetClientTlsConfigDhtNode(state), true, session);

	if (!session)
	{
		m_sessionCache.Put(addr, tmpSecComm->GetSessionCopy(), false);
	}

#endif //DECENT_DHT_NAIVE_RA_VER

	return CntPair(std::move(connection), std::move(tmpSecComm));
}
