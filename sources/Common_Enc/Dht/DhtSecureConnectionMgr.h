#pragma once

#include <DecentApi/Common/Net/SecureConnectionPoolBase.h>
#include <DecentApi/Common/Tools/SharedCachingQueue.h>

#ifdef DECENT_DHT_NAIVE_RA_VER
#	include <DecentApi/Common/SGX/RaTicket.h>
#endif

namespace Decent
{
	namespace MbedTlsObj
	{
		class Session;
	}

	namespace Dht
	{
		class DhtStates;

		class DhtSecureConnectionMgr
		{
		public:
			DhtSecureConnectionMgr(size_t maxOutCnt);

			virtual ~DhtSecureConnectionMgr();

			virtual Net::CntPair GetNew(const uint64_t& addr, DhtStates& state);

		private:
#ifdef DECENT_DHT_NAIVE_RA_VER
			Tools::SharedCachingQueue<uint64_t, const Sgx::RaClientSession> m_sessionCache;
#else
			Tools::SharedCachingQueue<uint64_t, MbedTlsObj::Session> m_sessionCache;
#endif
		};
	}
}
