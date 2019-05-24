#include "EntityList.h"

#include <iterator>

#include "EntityItem.h"
#include "ParseError.h"

using namespace Decent::Dht::AccessCtrl;

EntityList::EntityList()
{
}

EntityList::EntityList(std::vector<uint8_t>::const_iterator & it, std::vector<uint8_t>::const_iterator end)
{
	auto leftDist = std::distance(it, end);
	if (leftDist < 0 || (leftDist % EntityItem::Size() != 0))
	{
		throw ParseError("Failed to parse entity list. Buffer size doesn't match the size of entity item.");
	}

	while (it != end)
	{
		m_list.insert(m_list.end(), EntityItem(it, end));
	}
}

EntityList::~EntityList()
{
}

void EntityList::Merge(const EntityList & rhs)
{
	decltype(m_list) tmpList;

	std::set_union(m_list.begin(), m_list.end(),
		rhs.m_list.begin(), rhs.m_list.end(),
		std::inserter(tmpList, tmpList.end())
	);

	m_list.swap(tmpList);
}

bool EntityList::Search(const EntityItem & item) const
{
	return m_list.find(item) != m_list.end();
}

void EntityList::Insert(const EntityItem & item)
{
	m_list.insert(item);
}

size_t EntityList::GetSerializedSize() const
{
	return m_list.size() * EntityItem::Size();
}
