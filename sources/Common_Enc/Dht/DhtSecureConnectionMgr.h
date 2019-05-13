#pragma once

#include <DecentApi/Common/Net/SecureConnectionPoolBase.h>
#include <DecentApi/Common/Tools/SharedCachingQueue.h>

namespace Decent
{
	namespace MbedTlsObj
	{
		class Session;
	}
	namespace Ra
	{
		class States;
	}

	namespace Dht
	{
		class DhtSecureConnectionMgr
		{
		public:
			DhtSecureConnectionMgr(size_t maxOutCnt);

			virtual ~DhtSecureConnectionMgr();

			virtual Net::CntPair GetNew(const uint64_t& addr, Ra::States& state);

		private:
			Tools::SharedCachingQueue<uint64_t, MbedTlsObj::Session> m_sessionCache;
		};
	}
}
