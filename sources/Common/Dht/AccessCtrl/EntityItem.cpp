#include "EntityItem.h"

#include "ParseError.h"

using namespace Decent::Dht::AccessCtrl;

EntityItem::EntityItem(const general_256bit_hash & entityHash)
{
	std::copy(std::begin(entityHash), std::end(entityHash), m_hash);
}

EntityItem::EntityItem(std::vector<uint8_t>::const_iterator & it, std::vector<uint8_t>::const_iterator end)
{
	auto leftDist = std::distance(it, end);
	if (leftDist < 0 || static_cast<size_t>(leftDist) < Size())
	{
		throw ParseError("Failed to parse entity-based policy - entity.");
	}

	std::vector<uint8_t>::const_iterator firstStart = it;
	std::vector<uint8_t>::const_iterator firstEnd = (it += sizeof(m_hash));

	static_assert(EntityItem::Size() == sizeof(m_hash), "EntityItem buffer size doesn't match.");

	std::copy(firstStart, firstEnd, m_hash);
}

EntityItem::EntityItem(const EntityItem & rhs) :
	EntityItem(rhs.GetEntityHash())
{
}

EntityItem::~EntityItem()
{
}

EntityItem & EntityItem::operator=(const EntityItem & rhs)
{
	if (this != &rhs)
	{
		std::copy(std::begin(rhs.GetEntityHash()), std::end(rhs.GetEntityHash()), m_hash);
	}
	return *this;
}

void EntityItem::Swap(EntityItem & other)
{
	if (this != &other)
	{
		std::swap(m_hash, other.m_hash);
	}
}

const general_256bit_hash & EntityItem::GetEntityHash() const
{
	return m_hash;
}

size_t EntityItem::GetSize() const
{
	return EntityItem::Size();
}

const uint8_t * EntityItem::ByteBegin() const
{
	return reinterpret_cast<const uint8_t*>(m_hash);
}

const uint8_t * EntityItem::ByteEnd() const
{
	return reinterpret_cast<const uint8_t*>(m_hash) + sizeof(m_hash);
}
