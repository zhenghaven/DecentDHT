#pragma once

#include <string>

namespace Decent
{
	namespace Dht
	{
		namespace Tools
		{
			bool GetConfigurationJsonString(const std::string& filePath, std::string& outJsonStr);
		}
	}
}
