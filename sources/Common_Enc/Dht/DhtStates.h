#pragma once

#include <DecentApi/DecentAppEnclave/AppStates.h>
#include <DecentApi/Common/MbedTls/BigNumber.h>

namespace Decent
{
	namespace Net
	{
		class SecureConnectionPoolBase;
	}

	namespace Dht
	{
		template<typename IdType, size_t KeySizeByte, typename AddrType>
		class LocalNode;

		class EnclaveStore;
		class DhtConnectionPool;

		class DhtStates : public Ra::AppStates
		{
		public: //public member:
			static constexpr size_t sk_keySizeByte = static_cast<size_t>(32);

			typedef LocalNode<MbedTlsObj::BigNumber, sk_keySizeByte, uint64_t> DhtLocalNodeType;
			typedef std::shared_ptr<DhtLocalNodeType> DhtLocalNodePtrType;

		public:
			DhtStates(Ra::AppCertContainer & certCntnr, Ra::KeyContainer & keyCntnr, Ra::WhiteList::DecentServer & serverWl, GetLoadedWlFunc getLoadedFunc, EnclaveStore& dhtStore, DhtConnectionPool& cntPool, Net::SecureConnectionPoolBase& appCntPool) :
				AppStates(certCntnr, keyCntnr, serverWl, getLoadedFunc),
				m_dhtNode(),
				m_dhtStore(dhtStore),
				m_cntPool(cntPool),
				m_appCntPool(appCntPool)
			{}
			
			virtual ~DhtStates()
			{}

			const DhtLocalNodePtrType& GetDhtNode() const
			{
				return m_dhtNode;
			}

			DhtLocalNodePtrType& GetDhtNode()
			{
				return m_dhtNode;
			}

			const EnclaveStore& GetDhtStore() const
			{
				return m_dhtStore;
			}

			EnclaveStore& GetDhtStore()
			{
				return m_dhtStore;
			}

			const DhtConnectionPool& GetConnectionPool() const
			{
				return m_cntPool;
			}

			DhtConnectionPool& GetConnectionPool()
			{
				return m_cntPool;
			}

			const Net::SecureConnectionPoolBase& GetAppConnectionPool() const
			{
				return m_appCntPool;
			}

			Net::SecureConnectionPoolBase& GetAppConnectionPool()
			{
				return m_appCntPool;
			}

		private:
			DhtLocalNodePtrType m_dhtNode;
			EnclaveStore& m_dhtStore;
			DhtConnectionPool& m_cntPool;
			Net::SecureConnectionPoolBase& m_appCntPool;
		};
	}
}
