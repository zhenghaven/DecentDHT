#pragma once

#include <memory>
#include <cstdint>

namespace Decent
{
	namespace Dht
	{
		template<typename IdType, typename AddrType>
		class Node
		{
		public: //static members:
			typedef std::shared_ptr<Node> NodeBasePtrType;

		public:
			Node() = default;

			virtual ~Node()
			{}

			virtual NodeBasePtrType FindSuccessor(const IdType& key) = 0;

			virtual NodeBasePtrType FindPredecessor(const IdType& key) = 0;

			virtual NodeBasePtrType GetImmediateSuccessor() = 0;

			virtual NodeBasePtrType GetImmediatePredecessor() = 0;

			virtual void SetImmediatePredecessor(NodeBasePtrType pred) = 0;

			virtual void UpdateFingerTable(NodeBasePtrType& s, size_t i) = 0;

			virtual const IdType& GetNodeId() = 0;

			virtual const AddrType& GetAddress() = 0;

		};
	}
}
