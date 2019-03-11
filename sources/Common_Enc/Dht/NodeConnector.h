#pragma once

#include "../../Common/Dht/Node.h"
#include "../../Common/Dht/FuncNums.h"

#include <memory>

namespace Decent
{
	namespace MbedTlsObj
	{
		class BigNumber;
	}
	namespace Net
	{
		class TlsCommLayer;
	}

	namespace Dht
	{
		class NodeConnector : public Node<MbedTlsObj::BigNumber, uint64_t>
		{
		public:
			typedef Node<MbedTlsObj::BigNumber, uint64_t> NodeBaseType;
			typedef typename NodeBaseType::NodeBasePtrType NodeBasePtrType;

			static void SendNode(void* connection, Decent::Net::TlsCommLayer& tls, NodeBasePtrType node);

			static NodeBasePtrType ReceiveNode(void* connection, Decent::Net::TlsCommLayer& tls);

		public:
			NodeConnector() = delete;

			NodeConnector(uint64_t address);

			NodeConnector(uint64_t address, const MbedTlsObj::BigNumber& Id);

			NodeConnector(uint64_t address, MbedTlsObj::BigNumber&& Id);

			/** \brief	Destructor */
			virtual ~NodeConnector();

			virtual NodeBasePtrType LookupTypeFunc(const MbedTlsObj::BigNumber& key, EncFunc::Dht::NumType type);

			virtual NodeBasePtrType FindSuccessor(const MbedTlsObj::BigNumber& key) override;

			virtual NodeBasePtrType FindPredecessor(const MbedTlsObj::BigNumber& key) override;

			virtual NodeBasePtrType GetImmediateSuccessor() override;

			virtual NodeBasePtrType GetImmediatePredecessor() override;

			virtual void SetImmediatePredecessor(NodeBasePtrType pred) override;

			virtual void UpdateFingerTable(NodeBasePtrType& s, uint64_t i) override;

			virtual void DeUpdateFingerTable(const MbedTlsObj::BigNumber& oldId, NodeBasePtrType& succ, uint64_t i) override;

			virtual const MbedTlsObj::BigNumber& GetNodeId() override;

			virtual const uint64_t& GetAddress() override { return m_address; }

		private:
			uint64_t m_address;
			std::unique_ptr<MbedTlsObj::BigNumber> m_Id;
		};
	}
}
