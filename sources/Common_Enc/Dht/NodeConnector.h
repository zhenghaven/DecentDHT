#pragma once

#include "../../Common/Dht/NodeBase.h"
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
		class SecureCommLayer;
		class ConnectionBase;
	}

	namespace Dht
	{
		class NodeConnector : public NodeBase<MbedTlsObj::BigNumber, uint64_t>
		{
		public:
			typedef NodeBase<MbedTlsObj::BigNumber, uint64_t> NodeBaseType;
			typedef typename NodeBaseType::NodeBasePtr NodeBasePtr;

			static void SendNode(Decent::Net::SecureCommLayer& tls, NodeBasePtr node);
			static void SendNode(Decent::Net::ConnectionBase& connection, Decent::Net::SecureCommLayer& tls, NodeBasePtr node);

			static void SendAddress(Decent::Net::SecureCommLayer& tls, NodeBasePtr node);
			static void SendAddress(Decent::Net::ConnectionBase& connection, Decent::Net::SecureCommLayer& tls, NodeBasePtr node);

			static NodeBasePtr ReceiveNode(Decent::Net::SecureCommLayer& tls);
			static NodeBasePtr ReceiveNode(Decent::Net::ConnectionBase& connection, Decent::Net::SecureCommLayer& tls);

		public:
			NodeConnector() = delete;

			NodeConnector(uint64_t address);

			NodeConnector(uint64_t address, const MbedTlsObj::BigNumber& Id);

			NodeConnector(uint64_t address, MbedTlsObj::BigNumber&& Id);

			/** \brief	Destructor */
			virtual ~NodeConnector();

			virtual NodeBasePtr LookupTypeFunc(const MbedTlsObj::BigNumber& key, EncFunc::Dht::NumType type);

			virtual NodeBasePtr FindSuccessor(const MbedTlsObj::BigNumber& key) override;

			virtual NodeBasePtr FindPredecessor(const MbedTlsObj::BigNumber& key) override;

			virtual NodeBasePtr GetImmediateSuccessor() override;

			virtual NodeBasePtr GetImmediatePredecessor() override;

			virtual void SetImmediatePredecessor(NodeBasePtr pred) override;

			virtual void UpdateFingerTable(NodeBasePtr& s, uint64_t i) override;

			virtual void DeUpdateFingerTable(const MbedTlsObj::BigNumber& oldId, NodeBasePtr& succ, uint64_t i) override;

			virtual const MbedTlsObj::BigNumber& GetNodeId() override;

			virtual const uint64_t& GetAddress() const override { return m_address; }

		private:
			uint64_t m_address;
			std::unique_ptr<MbedTlsObj::BigNumber> m_Id;
		};
	}
}
