#include "EntityBasedControl.h"

#include <DecentApi/Common/make_unique.h>

#include "EntityList.h"
#include "ParseError.h"

using namespace Decent::Dht::AccessCtrl;

namespace
{
	static inline std::vector<uint8_t>::iterator WriteSize(std::vector<uint8_t>::iterator destIt, std::vector<uint8_t>::iterator end, const uint64_t size)
	{
		if (std::distance(destIt, end) < static_cast<int64_t>(sizeof(size)))
		{
			throw Decent::RuntimeException("No enough binary block space to serialize AbPolicy.");
		}

		const uint8_t* sizePtr = reinterpret_cast<const uint8_t*>(&size);

		return std::copy(sizePtr, sizePtr + sizeof(size), destIt);
	}

	static inline uint64_t ParseSize(std::vector<uint8_t>::const_iterator& srcIt, std::vector<uint8_t>::const_iterator end)
	{
		if (std::distance(srcIt, end) < static_cast<int64_t>(sizeof(uint64_t)))
		{
			throw ParseError("Failed to parse entity based control.");
		}

		uint64_t res = 0;
		uint8_t* sizePtr = reinterpret_cast<uint8_t*>(&res);

		std::vector<uint8_t>::const_iterator parseBegin = srcIt;
		std::vector<uint8_t>::const_iterator parseEnd = (srcIt += sizeof(res));
		std::copy(parseBegin, parseEnd, sizePtr);

		return res;
	}
}

EntityBasedControl::EntityBasedControl(std::unique_ptr<EntityList>&& r, std::unique_ptr<EntityList>&& w, std::unique_ptr<EntityList>&& x) :
	m_rPolicy(std::forward<std::unique_ptr<EntityList> >(r)),
	m_wPolicy(std::forward<std::unique_ptr<EntityList> >(w)),
	m_xPolicy(std::forward<std::unique_ptr<EntityList> >(x))
{
}

EntityBasedControl::EntityBasedControl(std::vector<uint8_t>::const_iterator & it, std::vector<uint8_t>::const_iterator end) :
	m_rPolicy(Tools::make_unique<EntityList>(it, it + ParseSize(it, end))),
	m_wPolicy(Tools::make_unique<EntityList>(it, it + ParseSize(it, end))),
	m_xPolicy(Tools::make_unique<EntityList>(it, it + ParseSize(it, end)))
{
}

EntityBasedControl::EntityBasedControl(EntityBasedControl && rhs) :
	m_rPolicy(std::forward<std::unique_ptr<EntityList> >(rhs.m_rPolicy)),
	m_wPolicy(std::forward<std::unique_ptr<EntityList> >(rhs.m_wPolicy)),
	m_xPolicy(std::forward<std::unique_ptr<EntityList> >(rhs.m_xPolicy))
{
}

EntityBasedControl::~EntityBasedControl()
{
}

size_t EntityBasedControl::GetSerializedSize() const
{
	return m_rPolicy->GetSerializedSize() +
		m_wPolicy->GetSerializedSize() +
		m_xPolicy->GetSerializedSize() +
		(3 * sizeof(uint64_t));
}

std::vector<uint8_t>::iterator EntityBasedControl::Serialize(std::vector<uint8_t>::iterator destIt, std::vector<uint8_t>::iterator end) const
{
	destIt = WriteSize(destIt, end, static_cast<uint64_t>(m_rPolicy->GetSerializedSize()));
	destIt = m_rPolicy->Serialize(destIt);

	destIt = WriteSize(destIt, end, static_cast<uint64_t>(m_wPolicy->GetSerializedSize()));
	destIt = m_wPolicy->Serialize(destIt);

	destIt = WriteSize(destIt, end, static_cast<uint64_t>(m_xPolicy->GetSerializedSize()));
	destIt = m_xPolicy->Serialize(destIt);

	return destIt;
}

bool EntityBasedControl::ExamineRead(const EntityItem & item) const
{
	return m_rPolicy->Search(item);
}

bool EntityBasedControl::ExamineWrite(const EntityItem & item) const
{
	return m_wPolicy->Search(item);
}

bool EntityBasedControl::ExamineExecute(const EntityItem & item) const
{
	return m_xPolicy->Search(item);
}
