#include "AbPolicyAnd.h"

#include <DecentApi/Common/make_unique.h>

using namespace Decent::Dht::AccessCtrl;

AbPolicyAnd::~AbPolicyAnd()
{
}

bool AbPolicyAnd::Examine(const AbAttributeList & attrList) const
{
	return m_left->Examine(attrList) && m_right->Examine(attrList);
}

AbPolicyAnd::FlagType AbPolicyAnd::GetFlagByte() const
{
	return sk_flag;
}

std::unique_ptr<AbPolicyBase> operator&(std::unique_ptr<AbPolicyBase>&& lhs, std::unique_ptr<AbPolicyBase>&& rhs)
{
	return Decent::Tools::make_unique<AbPolicyAnd>(std::forward<std::unique_ptr<AbPolicyBase> >(lhs), std::forward<std::unique_ptr<AbPolicyBase> >(rhs));
}
