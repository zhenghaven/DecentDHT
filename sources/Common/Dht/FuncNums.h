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
				constexpr NumType k_dUpdFingerTable = 7;
				constexpr NumType k_queryNonBlock   = 8;
				constexpr NumType k_queryReply      = 9;
			}

			namespace Store
			{
				typedef uint8_t NumType;
				constexpr NumType k_getMigrateData = 0;
				constexpr NumType k_setMigrateData = 1;
			}

			namespace App
			{
				typedef uint8_t NumType;
				constexpr NumType k_findSuccessor = 0;
				constexpr NumType k_getData       = 1;
				constexpr NumType k_setData       = 2;
				constexpr NumType k_delData       = 3;
			}
		}
	}
}
