#include "AbPolicyAny.h"

#include "ParseError.h"

using namespace Decent::Dht::AccessCtrl;

AbPolicyAny::AbPolicyAny(std::vector<uint8_t>::const_iterator & it, const std::vector<uint8_t>::const_iterator & end)
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

void AbPolicyAny::Serialize(std::vector<uint8_t>& output) const
{
	output.push_back(sk_flag);
}

void AbPolicyAny::GetRelatedAttributes(AbAttributeList & outputList) const
{
	return;
}
