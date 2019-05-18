#include "AbPolicyOr.h"

#include <DecentApi/Common/make_unique.h>

using namespace Decent::Dht::AccessCtrl;

AbPolicyOr::~AbPolicyOr()
{
}

bool AbPolicyOr::Examine(const AbAttributeList & attrList) const
{
	return m_left->Examine(attrList) || m_right->Examine(attrList);
}

AbPolicyOr::FlagType AbPolicyOr::GetFlagByte() const
{
	return sk_flag;
}

std::unique_ptr<AbPolicyBase> operator|(std::unique_ptr<AbPolicyBase>&& lhs, std::unique_ptr<AbPolicyBase>&& rhs)
{
	return Decent::Tools::make_unique<AbPolicyOr>(std::forward<std::unique_ptr<AbPolicyBase> >(lhs), std::forward<std::unique_ptr<AbPolicyBase> >(rhs));
}
