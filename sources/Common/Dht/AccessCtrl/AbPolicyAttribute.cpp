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

AbPolicyAttribute::AbPolicyAttribute(std::vector<uint8_t>::const_iterator & it, std::vector<uint8_t>::const_iterator end)
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

std::vector<uint8_t>::iterator AbPolicyAttribute::Serialize(std::vector<uint8_t>::iterator destIt, std::vector<uint8_t>::iterator end) const
{
	if (std::distance(destIt, end) < static_cast<int64_t>(GetSerializedSize()))
	{
		throw RuntimeException("No enough binary block space to serialize AbPolicy.");
	}

	*destIt++ = sk_flag;
	return m_attr->Serialize(destIt);
}

void AbPolicyAttribute::GetRelatedAttributes(AbAttributeList & outputList) const
{
	outputList.Insert(*m_attr);
}
