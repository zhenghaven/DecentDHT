#pragma once

#include "AbPolicyBase.h"

namespace Decent
{
	namespace Dht
	{
		namespace AccessCtrl
		{
			class AbPolicyBinaryOp : public AbPolicyBase
			{
			public: //Static members:
				typedef uint8_t FlagType;

			public:
				AbPolicyBinaryOp() = delete;

				AbPolicyBinaryOp(std::unique_ptr<AbPolicyBase>&& left, std::unique_ptr<AbPolicyBase>&& right);

				AbPolicyBinaryOp(const AbPolicyBinaryOp& rhs) = delete;

				AbPolicyBinaryOp(AbPolicyBinaryOp&& rhs);

				virtual ~AbPolicyBinaryOp();

				virtual bool Examine(const AbAttributeList& attrList) const = 0;

				virtual size_t GetSerializedSize() const override;

				virtual FlagType GetFlagByte() const = 0;

				virtual void Serialize(std::vector<uint8_t>& output) const override;

				virtual void GetRelatedAttributes(AbAttributeList& outputList) const override;

			protected:
				std::unique_ptr<AbPolicyBase> m_left;
				std::unique_ptr<AbPolicyBase> m_right;
			};
		}
	}
}
