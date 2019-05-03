#pragma once

#include <DecentApi/Common/Net/SecureConnectionPool.h>

namespace Decent
{
	namespace Dht
	{
		class DhtStates;

		class DhtConnectionPool : public Net::SecureConnectionPool<uint64_t>
		{
		public:
			DhtConnectionPool(size_t maxInCnt, size_t maxOutCnt) :
				SecureConnectionPool(maxInCnt, maxOutCnt)
			{}

			virtual ~DhtConnectionPool()
			{}

			virtual Net::CntPair GetNew(const uint64_t& addr, Ra::States& state) override;
		};
	}
}
