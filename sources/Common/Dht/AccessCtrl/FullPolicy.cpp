#include "FullPolicy.h"

#include <iterator>

#include "ParseError.h"

using namespace Decent::Dht::AccessCtrl;

FullPolicy::FullPolicy(std::vector<uint8_t>::const_iterator & it, std::vector<uint8_t>::const_iterator end) :
	m_owner(it, end),
	m_enclavePolicy(it, end),
	m_clientPolicy(it, end)
{
}

FullPolicy::FullPolicy(const EntityItem& owner, EntityBasedControl enclavePolicy, AttributeBasedControl clientPolicy) :
	m_owner(owner),
	m_enclavePolicy(std::move(enclavePolicy)),
	m_clientPolicy(std::move(clientPolicy))
{
}

Decent::Dht::AccessCtrl::FullPolicy::~FullPolicy()
{
}

size_t Decent::Dht::AccessCtrl::FullPolicy::GetSerializedSize() const
{
	return m_owner.GetSize() + 
		m_enclavePolicy.GetSerializedSize() +
		m_clientPolicy.GetSerializedSize();
}

std::vector<uint8_t>::iterator FullPolicy::Serialize(std::vector<uint8_t>::iterator destIt, std::vector<uint8_t>::iterator end) const
{
	if (std::distance(destIt, end) < static_cast<int64_t>(m_owner.GetSize()))
	{
		throw Decent::RuntimeException("No enough binary block space to serialize FullPolicy's owner entity.");
	}
	destIt = m_owner.Serialize(destIt);
	
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

bool FullPolicy::IsOwner(const EntityItem & testee) const
{
	return m_owner == testee;
}
