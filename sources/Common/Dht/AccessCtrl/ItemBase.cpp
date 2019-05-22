#include "ItemBase.h"

using namespace Decent::Dht::AccessCtrl;

bool ItemBase::operator==(const ItemBase & rhs) const
{
	return std::equal(ByteBegin(), ByteEnd(),
		rhs.ByteBegin());
}

bool ItemBase::operator>(const ItemBase & rhs) const
{
	return (rhs < *this);
}

bool ItemBase::operator>=(const ItemBase & rhs) const
{
	return !(*this < rhs);
}

bool ItemBase::operator<(const ItemBase & rhs) const
{
	return std::lexicographical_compare(ByteBegin(), ByteEnd(),
		rhs.ByteBegin(), rhs.ByteEnd());
}

bool ItemBase::operator<=(const ItemBase & rhs) const
{
	return !(*this > rhs);
}
