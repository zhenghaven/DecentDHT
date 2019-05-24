#include "AttributeBasedControl.h"

#include "AbPolicyBase.h"
#include "AbPolicyParse.h"

using namespace Decent::Dht::AccessCtrl;

AttributeBasedControl::AttributeBasedControl(std::vector<uint8_t>::const_iterator & it, std::vector<uint8_t>::const_iterator end) :
	m_rPolicy(Parse(it, end)),
	m_wPolicy(Parse(it, end)),
	m_xPolicy(Parse(it, end))
{
}

AttributeBasedControl::AttributeBasedControl(AttributeBasedControl && rhs) :
	m_rPolicy(std::forward<std::unique_ptr<AbPolicyBase> >(rhs.m_rPolicy)),
	m_wPolicy(std::forward<std::unique_ptr<AbPolicyBase> >(rhs.m_wPolicy)),
	m_xPolicy(std::forward<std::unique_ptr<AbPolicyBase> >(rhs.m_xPolicy))
{
}

AttributeBasedControl::AttributeBasedControl(std::unique_ptr<AbPolicyBase>&& r, std::unique_ptr<AbPolicyBase>&& w, std::unique_ptr<AbPolicyBase>&& x) :
	m_rPolicy(std::forward<std::unique_ptr<AbPolicyBase> >(r)),
	m_wPolicy(std::forward<std::unique_ptr<AbPolicyBase> >(w)),
	m_xPolicy(std::forward<std::unique_ptr<AbPolicyBase> >(x))
{
}

AttributeBasedControl::~AttributeBasedControl()
{
}

size_t AttributeBasedControl::GetSerializedSize() const
{
	return m_rPolicy->GetSerializedSize() +
		m_wPolicy->GetSerializedSize() +
		m_xPolicy->GetSerializedSize();
}

std::vector<uint8_t>::iterator AttributeBasedControl::Serialize(std::vector<uint8_t>::iterator destIt, std::vector<uint8_t>::iterator end) const
{
	destIt = m_rPolicy->Serialize(destIt, end);
	destIt = m_wPolicy->Serialize(destIt, end);
	destIt = m_xPolicy->Serialize(destIt, end);

	return destIt;
}

bool AttributeBasedControl::ExamineRead(const AbAttributeList & attrList) const
{
	return m_rPolicy->Examine(attrList);
}

bool AttributeBasedControl::ExamineWrite(const AbAttributeList & attrList) const
{
	return m_wPolicy->Examine(attrList);
}

bool AttributeBasedControl::ExamineExecute(const AbAttributeList & attrList) const
{
	return m_xPolicy->Examine(attrList);
}