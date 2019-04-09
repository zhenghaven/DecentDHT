#pragma once

#include <DecentApi/DecentAppEnclave/AppStates.h>
#include <DecentApi/Common/MbedTls/BigNumber.h>

namespace Decent
{
	namespace Dht
	{
		template<typename IdType, size_t KeySizeByte, typename AddrType>
		class LocalNode;

		class EnclaveStore;

		class DhtStates : public Ra::AppStates
		{
		public: //public member:
			static constexpr size_t sk_keySizeByte = static_cast<size_t>(32);

			typedef LocalNode<MbedTlsObj::BigNumber, sk_keySizeByte, uint64_t> DhtLocalNodeType;
			typedef std::shared_ptr<DhtLocalNodeType> DhtLocalNodePtrType;

		public:
			DhtStates(Ra::AppCertContainer & certCntnr, Ra::KeyContainer & keyCntnr, Ra::WhiteList::DecentServer & serverWl, GetLoadedWlFunc getLoadedFunc, EnclaveStore& dhtStore) :
				AppStates(certCntnr, keyCntnr, serverWl, getLoadedFunc),
				m_dhtNode(),
				m_dhtStore(dhtStore)
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

		private:
			DhtLocalNodePtrType m_dhtNode;
			EnclaveStore& m_dhtStore;
		};
	}
}
