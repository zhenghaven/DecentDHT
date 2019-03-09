#pragma once

#include <cstdint>
#include <string>

namespace Decent
{
	namespace Dht
	{
		namespace EncFunc
		{
			namespace Dht
			{
				typedef uint8_t NumType;
				constexpr NumType k_findSuccessor   = 0;
				constexpr NumType k_findPredecessor = 1;
				constexpr NumType k_getImmediateSuc = 2;
				constexpr NumType k_getNodeId       = 3;
			}
		}
	}
}
