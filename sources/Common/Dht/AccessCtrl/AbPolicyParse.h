#pragma once

#include "AbPolicyBase.h"

namespace Decent
{
	namespace Dht
	{
		namespace AccessCtrl
		{
			std::unique_ptr<AbPolicyBase> Parse(std::vector<uint8_t>::const_iterator& it, const std::vector<uint8_t>::const_iterator& end);
		}
	}
}
