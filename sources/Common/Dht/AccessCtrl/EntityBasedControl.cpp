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

	std::vector<uint8_t>::iterator SerializePolicy(const std::unique_ptr<EntityList>& p, std::vector<uint8_t>::iterator destIt, std::vector<uint8_t>::iterator end)
	{
		if (std::distance(destIt, end) < static_cast<int64_t>(sizeof(uint8_t)))
		{
			throw Decent::RuntimeException("No enough binary block space to serialize the EntityList of the EntityBasedControl.");
		}

		if (p)
		{
			*destIt++ = 0; //allow all : false
			destIt = WriteSize(destIt, end, static_cast<uint64_t>(p->GetSerializedSize()));
			destIt = p->Serialize(destIt);
		}
		else
		{
			*destIt++ = 1; //allow all : true
		}
		return destIt;
	}

	std::unique_ptr<EntityList> ParsePolicy(std::vector<uint8_t>::const_iterator& srcIt, std::vector<uint8_t>::const_iterator end)
	{
		if (std::distance(srcIt, end) < static_cast<int64_t>(sizeof(uint8_t)))
		{
			throw ParseError("Failed to parse entity based control.");
		}

		auto flag = *srcIt++;

		if (flag == 0) //allow all : false
		{
			return Decent::Tools::make_unique<EntityList>(srcIt, srcIt + ParseSize(srcIt, end));
		}
		else //allow all : true
		{
			return nullptr;
		}
	}
}

EntityBasedControl EntityBasedControl::AllowAll()
{
	return EntityBasedControl(nullptr, nullptr, nullptr);
}

EntityBasedControl EntityBasedControl::DenyAll()
{
	return EntityBasedControl(Tools::make_unique<EntityList>(), Tools::make_unique<EntityList>(), Tools::make_unique<EntityList>());
}

EntityBasedControl::EntityBasedControl(std::unique_ptr<EntityList> r, std::unique_ptr<EntityList> w, std::unique_ptr<EntityList> x) :
	m_rPolicy(std::move(r)),
	m_wPolicy(std::move(w)),
	m_xPolicy(std::move(x))
{
}

EntityBasedControl::EntityBasedControl(std::vector<uint8_t>::const_iterator & it, std::vector<uint8_t>::const_iterator end) :
	m_rPolicy(ParsePolicy(it, end)),
	m_wPolicy(ParsePolicy(it, end)),
	m_xPolicy(ParsePolicy(it, end))
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
	return (m_rPolicy ? sizeof(uint8_t) + sizeof(uint64_t) + m_rPolicy->GetSerializedSize() : sizeof(uint8_t)) +
		(m_wPolicy ? sizeof(uint8_t) + sizeof(uint64_t) + m_wPolicy->GetSerializedSize() : sizeof(uint8_t)) +
		(m_xPolicy ? sizeof(uint8_t) + sizeof(uint64_t) + m_xPolicy->GetSerializedSize() : sizeof(uint8_t));
}

std::vector<uint8_t>::iterator EntityBasedControl::Serialize(std::vector<uint8_t>::iterator destIt, std::vector<uint8_t>::iterator end) const
{
	destIt = SerializePolicy(m_rPolicy, destIt, end);
	destIt = SerializePolicy(m_wPolicy, destIt, end);
	destIt = SerializePolicy(m_xPolicy, destIt, end);

	return destIt;
}

bool EntityBasedControl::ExamineRead(const EntityItem & item) const
{
	return m_rPolicy ? m_rPolicy->Search(item) : true;
}

bool EntityBasedControl::ExamineWrite(const EntityItem & item) const
{
	return m_wPolicy ? m_wPolicy->Search(item) : true;
}

bool EntityBasedControl::ExamineExecute(const EntityItem & item) const
{
	return m_xPolicy ? m_xPolicy->Search(item) : true;
}
