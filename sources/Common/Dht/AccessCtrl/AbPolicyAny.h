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
				AbPolicyAny(std::vector<uint8_t>::const_iterator& it, const std::vector<uint8_t>::const_iterator& end);

				virtual ~AbPolicyAny();

				virtual bool Examine(const AbAttributeList& attrList) const override;

				virtual size_t GetSerializedSize() const override;

				virtual void Serialize(std::vector<uint8_t>& output) const override;

				virtual void GetRelatedAttributes(AbAttributeList& outputList) const override;
			};
		}
	}
}
