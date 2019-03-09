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

	namespace Dht
	{
		class NodeConnector : public Node<MbedTlsObj::BigNumber, uint64_t>
		{
		public:
			typedef Node<MbedTlsObj::BigNumber, uint64_t> NodeBaseType;

		public:
			NodeConnector() = delete;

			NodeConnector(uint64_t address);

			NodeConnector(uint64_t address, const MbedTlsObj::BigNumber& Id);

			NodeConnector(uint64_t address, MbedTlsObj::BigNumber&& Id);

			virtual ~NodeConnector();

			virtual NodeBaseType* LookupTypeFunc(const MbedTlsObj::BigNumber& key, EncFunc::Dht::NumType type);

			virtual NodeBaseType* FindSuccessor(const MbedTlsObj::BigNumber& key) override;

			virtual NodeBaseType* FindPredecessor(const MbedTlsObj::BigNumber& key) override;

			virtual NodeBaseType* GetImmediateSuccessor() override;

			virtual const MbedTlsObj::BigNumber& GetNodeId() override;

			virtual const uint64_t& GetAddress() override { return m_address; }

		private:
			uint64_t m_address;
			std::unique_ptr<MbedTlsObj::BigNumber> m_Id;
		};
	}
}
