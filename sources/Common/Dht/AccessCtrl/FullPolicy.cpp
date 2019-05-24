#include "FullPolicy.h"

using namespace Decent::Dht::AccessCtrl;

FullPolicy::FullPolicy(std::vector<uint8_t>::const_iterator & it, std::vector<uint8_t>::const_iterator end) :
	m_enclavePolicy(it, end),
	m_clientPolicy(it, end)
{
}

FullPolicy::FullPolicy(EntityBasedControl && enclavePolicy, AttributeBasedControl && clientPolicy) :
	m_enclavePolicy(std::forward<EntityBasedControl>(enclavePolicy)),
	m_clientPolicy(std::forward<AttributeBasedControl>(clientPolicy))
{
}

Decent::Dht::AccessCtrl::FullPolicy::~FullPolicy()
{
}

size_t Decent::Dht::AccessCtrl::FullPolicy::GetSerializedSize() const
{
	return m_enclavePolicy.GetSerializedSize() +
		m_clientPolicy.GetSerializedSize();
}

std::vector<uint8_t>::iterator Decent::Dht::AccessCtrl::FullPolicy::Serialize(std::vector<uint8_t>::iterator destIt, std::vector<uint8_t>::iterator end) const
{
	destIt = m_enclavePolicy.Serialize(destIt, end);
	destIt = m_clientPolicy.Serialize(destIt, end);
	return destIt;
}

EntityBasedControl & FullPolicy::GetEnclavePolicy()
{
	return m_enclavePolicy;
}

const EntityBasedControl & FullPolicy::GetEnclavePolicy() const
{
	return m_enclavePolicy;
}

AttributeBasedControl & FullPolicy::GetClientPolicy()
{
	return m_clientPolicy;
}

const AttributeBasedControl & FullPolicy::GetClientPolicy() const
{
	return m_clientPolicy;
}
