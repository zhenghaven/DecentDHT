#include "AbAttributeItem.h"

#include <cstring>

#include <algorithm>

#include "ParseError.h"

using namespace Decent::Dht::AccessCtrl;

AbAttributeItem::AbAttributeItem(const general_256bit_hash & userHash, const general_256bit_hash & attrHash)
{
	std::copy(std::begin(userHash), std::end(userHash), m_hashes[0]);
	std::copy(std::begin(attrHash), std::end(attrHash), m_hashes[1]);
}

AbAttributeItem::AbAttributeItem(std::vector<uint8_t>::const_iterator& it, const std::vector<uint8_t>::const_iterator& end)
{
	if (std::distance(it, end) < static_cast<int64_t>(AbAttributeItem::Size()))
	{
		throw ParseError("Failed to parse attributed-based policy - Attribute.");
	}

	std::vector<uint8_t>::const_iterator firstStart = it;
	std::vector<uint8_t>::const_iterator firstEnd   = (it += sizeof(m_hashes[0]));
	std::vector<uint8_t>::const_iterator secondEnd  = (it += sizeof(m_hashes[1]));

	static_assert(AbAttributeItem::Size() == sizeof(m_hashes[0]) + sizeof(m_hashes[1]), "AbAttributeItem buffer size doesn't match.");
	
	std::copy(firstStart, firstEnd,  m_hashes[0]);
	std::copy(firstEnd,   secondEnd, m_hashes[1]);
}

AbAttributeItem::AbAttributeItem(const AbAttributeItem & rhs) :
	AbAttributeItem(rhs.GetUserHash(), rhs.GetAttrHash())
{}

AbAttributeItem::~AbAttributeItem()
{
}

AbAttributeItem & AbAttributeItem::operator=(const AbAttributeItem & rhs)
{
	if (this != &rhs)
	{
		std::copy(std::begin(rhs.GetUserHash()), std::end(rhs.GetUserHash()), m_hashes[0]);
		std::copy(std::begin(rhs.GetAttrHash()), std::end(rhs.GetAttrHash()), m_hashes[1]);
	}
	return *this;
}

void AbAttributeItem::Swap(AbAttributeItem & other)
{
	if (this != &other)
	{
		std::swap(m_hashes[0], other.m_hashes[0]);
		std::swap(m_hashes[1], other.m_hashes[1]);
	}
}

const general_256bit_hash & AbAttributeItem::GetUserHash() const
{
	return m_hashes[0];
}

const general_256bit_hash & AbAttributeItem::GetAttrHash() const
{
	return m_hashes[1];
}

size_t AbAttributeItem::GetSize() const
{
	return AbAttributeItem::Size();
}

const uint8_t * AbAttributeItem::ByteBegin() const
{
	return reinterpret_cast<const uint8_t*>(m_hashes);
}

const uint8_t * AbAttributeItem::ByteEnd() const
{
	return reinterpret_cast<const uint8_t*>(m_hashes) + sizeof(m_hashes);
}
