#pragma once

#include <DecentApi/Common/Net/ConnectionPool.h>

namespace Decent
{
	namespace Dht
	{
		class DhtConnectionPool : public Net::ConnectionPool<uint64_t>
		{
		public:
			DhtConnectionPool(size_t maxInCnt, size_t maxOutCnt) :
				ConnectionPool(maxInCnt, maxOutCnt)
			{}

			virtual ~DhtConnectionPool()
			{}

			virtual std::unique_ptr<Net::ConnectionBase> GetNew(const uint64_t& addr) override;
		};
	}
}
