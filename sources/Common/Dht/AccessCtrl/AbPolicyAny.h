#pragma once

#include "AbPolicyBase.h"

namespace Decent
{
	namespace Dht
	{
		namespace AccessCtrl
		{
			class AbPolicyAny : public AbPolicyBase
			{
			public: //Static members:
				static constexpr uint8_t sk_flag = 'A';

				static constexpr size_t sk_seralizedSize = sizeof(sk_flag);

			public:
				AbPolicyAny() = default;

				/**
				 * \brief	Constructor for parsing policy stored in binary
				 *
				 * \param	it 	The iterator.
				 * \param	end	The end.
				 */
				AbPolicyAny(std::vector<uint8_t>::const_iterator& it, std::vector<uint8_t>::const_iterator end);

				virtual ~AbPolicyAny();

				virtual bool Examine(const AbAttributeList& attrList) const override;

				virtual size_t GetSerializedSize() const override;

				/**
				* \brief	Serialize this object to the given stream
				*
				* \param [out]	destIt	The iterator point to the begin of the binary block to insert the data.
				* \param 	   	end   	The end of the binary block.
				*
				* \return	A std::vector&lt;uint8_t&gt;::iterator, which points to the end of inserted data.
				*/
				virtual std::vector<uint8_t>::iterator Serialize(std::vector<uint8_t>::iterator destIt, std::vector<uint8_t>::iterator end) const override;

				virtual void GetRelatedAttributes(AbAttributeList& outputList) const override;
			};
		}
	}
}
