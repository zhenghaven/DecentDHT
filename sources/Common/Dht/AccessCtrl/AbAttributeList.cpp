#include "AbAttributeList.h"

#include <iterator>
#include <algorithm>

#include "ParseError.h"

using namespace Decent::Dht::AccessCtrl;

AbAttributeList::AbAttributeList()
{
}

AbAttributeList::AbAttributeList(std::vector<uint8_t>::const_iterator& it, std::vector<uint8_t>::const_iterator end)
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

AbAttributeList::AbAttributeList(AbAttributeList && rhs) :
	m_list(std::forward<std::set<AbAttributeItem> >(rhs.m_list))
{
}

AbAttributeList::AbAttributeList(std::set<AbAttributeItem> list) :
	m_list(std::move(list))
{
}

AbAttributeList::~AbAttributeList()
{
}

void AbAttributeList::Swap(AbAttributeList & rhs)
{
	m_list.swap(rhs.m_list);
}

void AbAttributeList::Insert(const AbAttributeItem & item)
{
	m_list.insert(item);
}

AbAttributeList AbAttributeList::Merge2New(const AbAttributeList & rhs) const
{
	AbAttributeList res;

	std::set_union(m_list.begin(), m_list.end(),
		rhs.m_list.begin(), rhs.m_list.end(),
		std::inserter(res.m_list, res.m_list.end())
	);

	return res;
}

void AbAttributeList::Merge(const AbAttributeList & rhs)
{
	auto tmpRes = Merge2New(rhs);

	Swap(tmpRes);
}

AbAttributeList AbAttributeList::Difference2New(const AbAttributeList & rhs) const
{
	AbAttributeList res;

	std::set_difference(m_list.begin(), m_list.end(),
		rhs.m_list.begin(), rhs.m_list.end(),
		std::inserter(res.m_list, res.m_list.end())
	);

	return res;
}

void AbAttributeList::Difference(const AbAttributeList & rhs)
{
	auto tmpRes = Difference2New(rhs);

	Swap(tmpRes);
}

AbAttributeList AbAttributeList::operator+(const AbAttributeList & rhs) const
{
	return Merge2New(rhs);
}

AbAttributeList AbAttributeList::operator-(const AbAttributeList & rhs) const
{
	return Difference2New(rhs);
}

AbAttributeList & AbAttributeList::operator+=(const AbAttributeList & rhs)
{
	Merge(rhs);
	return *this;
}

AbAttributeList & AbAttributeList::operator-=(const AbAttributeList & rhs)
{
	Difference(rhs);
	return *this;
}

bool AbAttributeList::Search(const AbAttributeItem & item) const
{
	return m_list.find(item) != m_list.end();
}

size_t AbAttributeList::GetSerializedSize() const
{
	return m_list.size() * AbAttributeItem::Size();
}

size_t AbAttributeList::GetSize() const
{
	return m_list.size();
}
