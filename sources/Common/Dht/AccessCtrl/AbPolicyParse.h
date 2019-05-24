#pragma once

#include "AbPolicyBase.h"

namespace Decent
{
	namespace Dht
	{
		namespace AccessCtrl
		{
			/**
			 * \brief	Parses the attributed policy from binary block. Note: Parsing in here is not simply parse till the end, instead, it will stop
			 * 			when enough data is parsed.
			 *
			 * \param	it 	The iterator point to the beginning of the binary block.
			 * \param	end	The end of the binary block.
			 *
			 * \return	A std::unique_ptr&lt;AbPolicyBase&gt;
			 */
			std::unique_ptr<AbPolicyBase> Parse(std::vector<uint8_t>::const_iterator& it, std::vector<uint8_t>::const_iterator end);
		}
	}
}
