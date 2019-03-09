#pragma once

#include <cstdint>

namespace Decent
{
	namespace Dht
	{
		template<typename ConstIdType, typename AddrType>
		class Node
		{
		public:
			Node() = default;

			virtual ~Node()
			{}

			virtual const Node* FindSuccessor(ConstIdType key) const = 0;

			virtual const Node* FindPredecessor(ConstIdType key) const = 0;

			virtual const Node* GetImmediateSuccessor() const = 0;

			virtual ConstIdType& GetNodeId() const = 0;

			virtual const AddrType& GetAddress() const = 0;

		};
	}
}
