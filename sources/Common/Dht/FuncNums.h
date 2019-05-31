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
				constexpr NumType k_listQueryNonBlock = 10;
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
				constexpr NumType k_readData      = 1;
				constexpr NumType k_insertData    = 2;
				constexpr NumType k_updateData    = 3;
				constexpr NumType k_delData       = 4;
				//constexpr NumType k_findAtListSuc = 5;
				//constexpr NumType k_insertAttList = 6;
			}

			namespace User
			{
				typedef uint8_t NumType;
				constexpr NumType k_findSuccessor = 0;
				constexpr NumType k_readData      = 1;
				constexpr NumType k_insertData    = 2;
				constexpr NumType k_updateData    = 3;
				constexpr NumType k_delData       = 4;
				constexpr NumType k_findAtListSuc = 5;
				constexpr NumType k_insertAttList = 6;
			}

			namespace FileOpRet
			{
				typedef uint8_t NumType;
				constexpr NumType k_success  = 0;
				constexpr NumType k_nonExist = 1;
				constexpr NumType k_denied   = 2;
				constexpr NumType k_dos      = 3;
			}
		}
	}
}
