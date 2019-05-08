#pragma once

#include <DecentApi/CommonApp/Net/ConnectionPool.h>

namespace Decent
{
	namespace Dht
	{
		class DhtConnectionPool : public Net::ConnectionPool<uint64_t>
		{
		public:
			DhtConnectionPool(size_t maxInCnt, size_t maxOutCnt, size_t cntPoolWorker) :
				ConnectionPool(maxInCnt, maxOutCnt, cntPoolWorker)
			{}

			virtual ~DhtConnectionPool()
			{}

			virtual std::unique_ptr<Net::ConnectionBase> GetNew(const uint64_t& addr) override;
		};
	}
}
