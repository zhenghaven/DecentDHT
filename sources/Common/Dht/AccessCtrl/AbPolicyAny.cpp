#include "AbPolicyAny.h"

#include "ParseError.h"

using namespace Decent::Dht::AccessCtrl;

AbPolicyAny::AbPolicyAny(std::vector<uint8_t>::const_iterator & it, std::vector<uint8_t>::const_iterator end)
{
	if (it == end || *it != sk_flag)
	{
		throw ParseError("Failed to parse attributed-based policy - Any.");
	}
	++it;
}

AbPolicyAny::~AbPolicyAny()
{
}

bool AbPolicyAny::Examine(const AbAttributeList & attrList) const
{
	return true;
}

size_t AbPolicyAny::GetSerializedSize() const
{
	return sk_seralizedSize;
}

std::vector<uint8_t>::iterator AbPolicyAny::Serialize(std::vector<uint8_t>::iterator destIt, std::vector<uint8_t>::iterator end) const
{
	if (std::distance(destIt, end) < static_cast<int64_t>(sizeof(sk_flag)))
	{
		throw RuntimeException("No enough binary block space to serialize AbPolicy.");
	}

	*destIt++ = sk_flag;
	return destIt;
}

void AbPolicyAny::GetRelatedAttributes(AbAttributeList & outputList) const
{
	return;
}
