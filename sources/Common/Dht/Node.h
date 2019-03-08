#pragma once

#include <cstdint>

namespace Decent
{
	namespace Dht
	{
		template<typename ConstIdType>
		class Node
		{
		public:
			Node() = default;

			virtual ~Node()
			{}

			virtual uint64_t FindSuccessor(ConstIdType key) const = 0;

		};
	}
}
