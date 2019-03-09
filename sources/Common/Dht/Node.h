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

			virtual Node* FindSuccessor(const IdType& key) = 0;

			virtual Node* FindPredecessor(const IdType& key) = 0;

			virtual Node* GetImmediateSuccessor() = 0;

			virtual const IdType& GetNodeId() = 0;

			virtual const AddrType& GetAddress() = 0;

		};
	}
}
