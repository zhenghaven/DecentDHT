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
				constexpr NumType k_getNodeId       = 0;
				constexpr NumType k_findSuccessor   = 1;
				constexpr NumType k_findPredecessor = 2;
				constexpr NumType k_getImmediateSuc = 3;
				constexpr NumType k_getImmediatePre = 4;
				constexpr NumType k_setImmediatePre = 5;
				constexpr NumType k_updFingerTable  = 6;
				constexpr NumType k_dUpdFingerTable  = 7;
			}
		}
	}
}
