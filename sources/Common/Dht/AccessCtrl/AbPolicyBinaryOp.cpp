#include "AbPolicyBinaryOp.h"

#include "ParseError.h"

using namespace Decent::Dht::AccessCtrl;

AbPolicyBinaryOp::AbPolicyBinaryOp(std::unique_ptr<AbPolicyBase>&& left, std::unique_ptr<AbPolicyBase>&& right) :
	m_left(std::forward<std::unique_ptr<AbPolicyBase> >(left)),
	m_right(std::forward<std::unique_ptr<AbPolicyBase> >(right))
{
}

AbPolicyBinaryOp::AbPolicyBinaryOp(AbPolicyBinaryOp && rhs) :
	m_left(std::forward<std::unique_ptr<AbPolicyBase> >(rhs.m_left)),
	m_right(std::forward<std::unique_ptr<AbPolicyBase> >(rhs.m_right))
{
}

AbPolicyBinaryOp::~AbPolicyBinaryOp()
{
}

size_t AbPolicyBinaryOp::GetSerializedSize() const
{
	return sizeof(FlagType) + m_left->GetSerializedSize() + m_right->GetSerializedSize();
}

std::vector<uint8_t>::iterator AbPolicyBinaryOp::Serialize(std::vector<uint8_t>::iterator destIt, std::vector<uint8_t>::iterator end) const
{
	if (std::distance(destIt, end) < static_cast<int64_t>(sizeof(FlagType)))
	{
		throw RuntimeException("No enough binary block space to serialize AbPolicy.");
	}

	*destIt++ = GetFlagByte();
	destIt = m_left->Serialize(destIt, end);
	destIt = m_right->Serialize(destIt, end);
	return destIt;
}

void AbPolicyBinaryOp::GetRelatedAttributes(AbAttributeList & outputList) const
{
	m_left->GetRelatedAttributes(outputList);
	m_right->GetRelatedAttributes(outputList);
}
