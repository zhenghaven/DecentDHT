#pragma once

#include "AbPolicyBinaryOp.h"

namespace Decent
{
	namespace Dht
	{
		namespace AccessCtrl
		{
			class AbPolicyOr : public AbPolicyBinaryOp
			{
			public: //Static members:
				static constexpr FlagType sk_flag = '|';

			public:
				using AbPolicyBinaryOp::AbPolicyBinaryOp;

				virtual ~AbPolicyOr();

				/**
				* \brief	Examines the given attribute list. m_left &amp;&amp; m_right. Follows short circuit
				* 			rules.
				*
				* \param	attrList	List of attributes.
				*
				* \return	True if it succeeds, false if it fails.
				*/
				virtual bool Examine(const AbAttributeList& attrList) const override;

				virtual FlagType GetFlagByte() const override;
			};
		}
	}
}

std::unique_ptr<Decent::Dht::AccessCtrl::AbPolicyBase> operator|(std::unique_ptr<Decent::Dht::AccessCtrl::AbPolicyBase>&& lhs,
	std::unique_ptr<Decent::Dht::AccessCtrl::AbPolicyBase>&& rhs);
