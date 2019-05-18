#include "AbPolicyNot.h"

#include <DecentApi/Common/make_unique.h>

#include "AbPolicyAny.h"

using namespace Decent::Dht::AccessCtrl;

std::unique_ptr<AbPolicyBase> AbPolicyNot::DenyAll()
{
	return Tools::make_unique<AbPolicyNot>(Tools::make_unique<AbPolicyAny>());
}

AbPolicyNot::~AbPolicyNot()
{
}

bool AbPolicyNot::Examine(const AbAttributeList & attrList) const
{
	return !m_var->Examine(attrList);
}

AbPolicyNot::FlagType AbPolicyNot::GetFlagByte() const
{
	return sk_flag;
}

std::unique_ptr<AbPolicyBase> operator~(std::unique_ptr<AbPolicyBase>&& var)
{
	return Decent::Tools::make_unique<AbPolicyNot>(std::forward<std::unique_ptr<AbPolicyBase> >(var));
}
