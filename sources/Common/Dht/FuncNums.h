#pragma once

#include <cstdint>
#include <string>

namespace Decent
{
	namespace Dht
	{
		namespace EncFunc
		{
			template<typename T>
			inline std::string GetMsgString(const T num)
			{
				return std::string(&num, &num + sizeof(num));
			}

			template<typename T>
			inline T GetNum(const std::string& msg)
			{
				return msg.size() == sizeof(T) ? *reinterpret_cast<const T*>(msg.data()) : -1;
			}

			namespace Dht
			{
				
				typedef uint8_t NumType;
				constexpr NumType k_lookup   = 0;
				constexpr NumType k_leave    = 1;
				constexpr NumType k_join     = 2;
			}


		}
	}
}
