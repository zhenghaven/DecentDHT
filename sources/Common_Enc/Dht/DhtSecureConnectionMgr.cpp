#include "DhtSecureConnectionMgr.h"

#include <DecentApi/Common/make_unique.h>
#include <DecentApi/Common/Net/ConnectionBase.h>
#include <DecentApi/Common/Net/TlsCommLayer.h>
#include <DecentApi/Common/Tools/SharedCachingQueue.h>
#include <DecentApi/Common/MbedTls/Session.h>

#include <DecentApi/CommonEnclave/Ra/TlsConfigSameEnclave.h>

#include "ConnectionManager.h"
#include "DhtStates.h"

#ifndef DECENT_DHT_NAIVE_RA_VER
#	define DECENT_DHT_NAIVE_RA_VER
#endif // !DECENT_DHT_NAIVE_RA_VER

#if defined(DECENT_DHT_NAIVE_RA_VER) && defined(ENCLAVE_PLATFORM_SGX)
#include <DecentApi/Common/SGX/RaProcessorSp.h>
#include <DecentApi/Common/SGX/RaSpCommLayer.h>
#include <DecentApi/CommonEnclave/SGX/RaProcessorClient.h>
#include <DecentApi/CommonEnclave/SGX/RaClientCommLayer.h>
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

#if defined(DECENT_DHT_NAIVE_RA_VER) && defined(ENCLAVE_PLATFORM_SGX)
	std::unique_ptr<Sgx::RaProcessorClient> client =
		Tools::make_unique<Sgx::RaProcessorClient>(state.GetEnclaveId(), Sgx::RaProcessorClient::sk_acceptAnyPubKey, Sgx::RaProcessorClient::sk_acceptAnyRaConfig);

	std::unique_ptr<Sgx::RaClientCommLayer> tmpSecComm =
		Tools::make_unique<Sgx::RaClientCommLayer>(*connection, client);


#else
	std::shared_ptr<MbedTlsObj::Session> session = m_sessionCache.Get(addr);

	std::unique_ptr<TlsCommLayer> tmpSecComm = Tools::make_unique<TlsCommLayer>(*connection, GetClientTlsConfigDhtNode(state), true, session);

	if (!session)
	{
		m_sessionCache.Put(addr, tmpSecComm->GetSessionCopy(), false);
	}
#endif //DECENT_DHT_NAIVE_RA_VER

	std::unique_ptr<SecureCommLayer> comm = std::move(tmpSecComm);
	return CntPair(connection, comm);
}
