#pragma once

#include "AbPolicyBase.h"

namespace Decent
{
	namespace Dht
	{
		namespace AccessCtrl
		{
			class AbPolicyAttribute : public AbPolicyBase
			{
			public: //Static members:
				static constexpr uint8_t sk_flag = 'V';

				static constexpr size_t sk_seralizedSize = sizeof(sk_flag) + AbAttributeItem::Size();

			public:
				AbPolicyAttribute() = delete;

				AbPolicyAttribute(const AbAttributeItem& attr);

				AbPolicyAttribute(const AbPolicyAttribute& rhs);

				AbPolicyAttribute(AbPolicyAttribute&& rhs);

				/**
				* \brief	Constructor for parsing policy stored in binary
				*
				* \param	it 	The iterator.
				* \param	end	The end.
				*/
				AbPolicyAttribute(std::vector<uint8_t>::const_iterator& it, const std::vector<uint8_t>::const_iterator& end);

				virtual ~AbPolicyAttribute();

				virtual bool Examine(const AbAttributeList& attrList) const override;

				virtual size_t GetSerializedSize() const override;

				virtual void Serialize(std::vector<uint8_t>& output) const override;

				virtual void GetRelatedAttributes(AbAttributeList& outputList) const override;

			private:
				std::unique_ptr<AbAttributeItem> m_attr;
			};
		}
	}
}
