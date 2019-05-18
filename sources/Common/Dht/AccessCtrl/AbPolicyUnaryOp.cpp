#include "AbPolicyUnaryOp.h"

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

void AbPolicyUnaryOp::Serialize(std::vector<uint8_t>& output) const
{
	output.push_back(GetFlagByte());
	m_var->Serialize(output);
}

void AbPolicyUnaryOp::GetRelatedAttributes(AbAttributeList & outputList) const
{
	m_var->GetRelatedAttributes(outputList);
}
