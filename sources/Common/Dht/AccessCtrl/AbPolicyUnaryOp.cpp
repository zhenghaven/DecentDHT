#include "AbPolicyUnaryOp.h"

#include "ParseError.h"

using namespace Decent::Dht::AccessCtrl;

AbPolicyUnaryOp::AbPolicyUnaryOp(std::unique_ptr<AbPolicyBase>&& var) :
	m_var(std::forward<std::unique_ptr<AbPolicyBase> >(var))
{
}

AbPolicyUnaryOp::AbPolicyUnaryOp(AbPolicyUnaryOp && rhs) :
	m_var(std::forward<std::unique_ptr<AbPolicyBase> >(rhs.m_var))
{
}

AbPolicyUnaryOp::~AbPolicyUnaryOp()
{
}

size_t AbPolicyUnaryOp::GetSerializedSize() const
{
	return sizeof(FlagType) + m_var->GetSerializedSize();
}

std::vector<uint8_t>::iterator AbPolicyUnaryOp::Serialize(std::vector<uint8_t>::iterator destIt, std::vector<uint8_t>::iterator end) const
{
	if (std::distance(destIt, end) < static_cast<int64_t>(sizeof(FlagType)))
	{
		throw RuntimeException("No enough binary block space to serialize AbPolicy.");
	}

	*destIt++ = GetFlagByte();
	destIt = m_var->Serialize(destIt, end);
	return destIt;
}

void AbPolicyUnaryOp::GetRelatedAttributes(AbAttributeList & outputList) const
{
	m_var->GetRelatedAttributes(outputList);
}
