#pragma once

#include <DecentApi/DecentAppEnclave/AppStates.h>
#include <DecentApi/Common/MbedTls/BigNumber.h>

#include "../../Common/Dht/LocalNode.h"

namespace Decent
{
	namespace Dht
	{
		class DhtStates : public Ra::AppStates
		{
		public: //public member:
			static constexpr size_t sk_keySizeByte = static_cast<size_t>(32);

			typedef LocalNode<MbedTlsObj::BigNumber, const MbedTlsObj::BigNumber, sk_keySizeByte, uint64_t> DhtNodeType;

		public:
			DhtStates(Ra::AppCertContainer & certCntnr, Ra::KeyContainer & keyCntnr, Ra::WhiteList::DecentServer & serverWl, const Ra::WhiteList::HardCoded & hardCodedWl, GetLoadedWlFunc getLoadedFunc) :
				AppStates(certCntnr, keyCntnr, serverWl, hardCodedWl, getLoadedFunc),
				m_dhtNode()
			{}
			
			virtual ~DhtStates()
			{}

			const std::unique_ptr<DhtNodeType>& GetDhtNode() const
			{
				return m_dhtNode;
			}

			std::unique_ptr<DhtNodeType>& GetDhtNode()
			{
				return m_dhtNode;
			}

		private:
			std::unique_ptr<DhtNodeType> m_dhtNode;
		};
	}
}
