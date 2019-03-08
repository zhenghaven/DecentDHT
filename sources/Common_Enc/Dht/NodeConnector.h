#pragma once

#include "../../Common/Dht/Node.h"

namespace Decent
{
	namespace Dht
	{
		template<typename ConstIdType>
		class NodeConnector : public Node
		{
		public:
			NodeConnector() = delete;

			NodeConnector(uint64_t address) :
				m_address(address)
			{}

			virtual ~NodeConnector() 
			{}

			virtual uint64_t FindSuccessor(ConstIdType key) const override
			{

			}

		private:
			uint64_t m_address;

		};
	}
}
