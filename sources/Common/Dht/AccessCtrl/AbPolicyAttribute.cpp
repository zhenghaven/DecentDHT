#include "AbPolicyAttribute.h"

#include <algorithm>

#include <DecentApi/Common/make_unique.h>

#include "ParseError.h"

using namespace Decent::Dht::AccessCtrl;

AbPolicyAttribute::AbPolicyAttribute(const AbAttributeItem & attr) :
	m_attr(Tools::make_unique<AbAttributeItem>(attr))
{
}

AbPolicyAttribute::AbPolicyAttribute(const AbPolicyAttribute & rhs) :
	m_attr(Tools::make_unique<AbAttributeItem>(*rhs.m_attr))
{
}

AbPolicyAttribute::AbPolicyAttribute(AbPolicyAttribute && rhs) :
	m_attr(std::forward<std::unique_ptr<AbAttributeItem> >(rhs.m_attr))
{
}

AbPolicyAttribute::AbPolicyAttribute(std::vector<uint8_t>::const_iterator & it, const std::vector<uint8_t>::const_iterator & end)
{
	if (it == end || *it != sk_flag)
	{
		throw ParseError("Failed to parse attributed-based policy - Attribute.");
	}

	++it; //skip flag.

	m_attr = Tools::make_unique<AbAttributeItem>(it, end);
}

AbPolicyAttribute::~AbPolicyAttribute()
{
}

bool AbPolicyAttribute::Examine(const AbAttributeList & attrList) const
{
	return attrList.Search(*m_attr);
}

size_t AbPolicyAttribute::GetSerializedSize() const
{
	return sk_seralizedSize;
}

void AbPolicyAttribute::Serialize(std::vector<uint8_t>& output) const
{
	output.push_back(sk_flag);
	m_attr->Serialize(std::back_inserter(output));
}

void AbPolicyAttribute::GetRelatedAttributes(AbAttributeList & outputList) const
{
	outputList.Insert(*m_attr);
}
