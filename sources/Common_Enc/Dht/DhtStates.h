#pragma once

#include <DecentApi/DecentAppEnclave/AppStates.h>
#include <DecentApi/Common/MbedTls/BigNumber.h>

namespace Decent
{
	namespace Dht
	{
		template<typename IdType, size_t KeySizeByte, typename AddrType>
		class LocalNode;

		class DhtStore;

		class DhtStates : public Ra::AppStates
		{
		public: //public member:
			static constexpr size_t sk_keySizeByte = static_cast<size_t>(32);

			typedef LocalNode<MbedTlsObj::BigNumber, sk_keySizeByte, uint64_t> DhtLocalNodeType;
			typedef std::shared_ptr<DhtLocalNodeType> DhtLocalNodePtrType;

		public:
			DhtStates(Ra::AppCertContainer & certCntnr, Ra::KeyContainer & keyCntnr, Ra::WhiteList::DecentServer & serverWl, const Ra::WhiteList::HardCoded & hardCodedWl, GetLoadedWlFunc getLoadedFunc, DhtStore& dhtStore) :
				AppStates(certCntnr, keyCntnr, serverWl, hardCodedWl, getLoadedFunc),
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

			const DhtStore& GetDhtStore() const
			{
				return m_dhtStore;
			}

			DhtStore& GetDhtStore()
			{
				return m_dhtStore;
			}

		private:
			DhtLocalNodePtrType m_dhtNode;
			DhtStore& m_dhtStore;
		};
	}
}
