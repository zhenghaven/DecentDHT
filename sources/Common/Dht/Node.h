#pragma once

#include <cstdint>

namespace Decent
{
	namespace Dht
	{
		template<typename IdType, typename AddrType>
		class Node
		{
		public:
			Node() = default;

			virtual ~Node()
			{}

			virtual const Node* FindSuccessor(const IdType key) const = 0;

			virtual const Node* FindPredecessor(const IdType key) const = 0;

			virtual const Node* GetImmediateSuccessor() const = 0;

			virtual const IdType& GetNodeId() const = 0;

			virtual const AddrType& GetAddress() const = 0;

		};
	}
}
