#include "AbAttributeList.h"

#include <iterator>
#include <algorithm>

#include "ParseError.h"

using namespace Decent::Dht::AccessCtrl;

AbAttributeList::AbAttributeList()
{
}

AbAttributeList::AbAttributeList(std::vector<uint8_t>::const_iterator& it, const std::vector<uint8_t>::const_iterator& end)
{
	auto leftDist = std::distance(it, end);
	if (leftDist < 0 || (leftDist % AbAttributeItem::Size() != 0))
	{
		throw ParseError("Failed to parse attribute list. Buffer size doesn't match the size of attribute item.");
	}

	while (it != end)
	{
		m_list.insert(m_list.end(), AbAttributeItem(it, end));
	}
}

AbAttributeList::~AbAttributeList()
{
}

void AbAttributeList::Insert(const AbAttributeItem & item)
{
	m_list.insert(item);
}

void AbAttributeList::Merge(const AbAttributeList & rhs)
{
	decltype(m_list) tmpList;

	std::set_union(m_list.begin(), m_list.end(),
		rhs.m_list.begin(), rhs.m_list.end(),
		std::inserter(tmpList, tmpList.end())
	);
	
	m_list.swap(tmpList);
}

bool AbAttributeList::Search(const AbAttributeItem & item) const
{
	return m_list.find(item) != m_list.end();
}

size_t AbAttributeList::GetSerializedSize() const
{
	return m_list.size() * AbAttributeItem::Size();
}
