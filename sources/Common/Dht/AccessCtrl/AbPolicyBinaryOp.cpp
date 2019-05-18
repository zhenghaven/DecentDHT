#include "AbPolicyBinaryOp.h"

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

void AbPolicyBinaryOp::Serialize(std::vector<uint8_t>& output) const
{
	output.push_back(GetFlagByte());
	m_left->Serialize(output);
	m_right->Serialize(output);
}

void AbPolicyBinaryOp::GetRelatedAttributes(AbAttributeList & outputList) const
{
	m_left->GetRelatedAttributes(outputList);
	m_right->GetRelatedAttributes(outputList);
}
