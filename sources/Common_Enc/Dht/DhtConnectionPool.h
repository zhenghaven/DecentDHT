#pragma once

#include <DecentApi/Common/Net/TlsConnectionPool.h>

#include <map>
#include <memory>

namespace Decent
{
	namespace Net
	{
		class TlsCommLayer;
		class EnclaveNetConnector;
	}

	namespace Dht
	{
		class DhtStates;

		class DhtCntPair
		{
		public:
			DhtCntPair(std::unique_ptr<Net::EnclaveNetConnector>&& cnt, std::unique_ptr<Net::TlsCommLayer>&& tls);

			DhtCntPair(std::unique_ptr<Net::EnclaveNetConnector>& cnt, std::unique_ptr<Net::TlsCommLayer>& tls);

			//Copy is not allowed
			DhtCntPair(const DhtCntPair&) = delete;

			DhtCntPair(DhtCntPair&& rhs);

			virtual ~DhtCntPair();

			virtual Net::TlsCommLayer & GetTlsCommLayer();

			virtual Net::EnclaveNetConnector & GetConnection();

			DhtCntPair& operator=(const DhtCntPair& rhs) = delete;

			DhtCntPair& operator=(DhtCntPair&& rhs);

			DhtCntPair& Swap(DhtCntPair& other);

		private:
			std::unique_ptr<Net::EnclaveNetConnector> m_cnt;
			std::unique_ptr<Net::TlsCommLayer> m_tls;
		};

		class DhtConnectionPool : public Net::TlsConnectionPool
		{
		public: //static member:
			typedef std::pair<std::mutex, std::vector<DhtCntPair> > MapItemType;
			typedef std::map<uint64_t, MapItemType> MapType;

		public:
			DhtConnectionPool(size_t maxInCnt, size_t maxOutCnt);

			virtual ~DhtConnectionPool();

			void Put(uint64_t addr, DhtCntPair& cntPair)
			{
				return Put(addr, std::move(cntPair));
			}

			void Put(uint64_t addr, DhtCntPair&& cntPair);

			DhtCntPair GetNew(uint64_t addr, DhtStates& state);

			DhtCntPair Get(uint64_t addr, DhtStates& state);

		private:
			std::mutex m_mapMutex;
			MapType m_map;
		};
	}
}
