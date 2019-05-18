#pragma once

#include "AbPolicyUnaryOp.h"

namespace Decent
{
	namespace Dht
	{
		namespace AccessCtrl
		{
			class AbPolicyNot : public AbPolicyUnaryOp
			{
			public: //Static members:
				static constexpr FlagType sk_flag = '~';

				static std::unique_ptr<AbPolicyBase> DenyAll();

			public:
				using AbPolicyUnaryOp::AbPolicyUnaryOp;

				virtual ~AbPolicyNot();

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

std::unique_ptr<Decent::Dht::AccessCtrl::AbPolicyBase> operator~(std::unique_ptr<Decent::Dht::AccessCtrl::AbPolicyBase>&& var);
