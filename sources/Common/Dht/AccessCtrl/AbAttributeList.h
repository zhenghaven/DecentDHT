#pragma once

#include <set>
#include <vector>

#include "AbAttributeItem.h"

namespace Decent
{
	namespace Dht
	{
		namespace AccessCtrl
		{
			class AbAttributeList
			{
			public:
				AbAttributeList();

				/**
				 * \brief	Constructor that parse Attribute item from binary array. Note: distance(it, end)
				 * 			should be exact size of (N * AbAttributeItem::Size()).
				 *
				 * \param	it 	The iterator.
				 * \param	end	The end.
				 */
				AbAttributeList(std::vector<uint8_t>::const_iterator& it, std::vector<uint8_t>::const_iterator end);

				AbAttributeList(AbAttributeList&& rhs);

				AbAttributeList(std::set<AbAttributeItem> list);

				virtual ~AbAttributeList();

				void Swap(AbAttributeList& rhs);

				void Insert(const AbAttributeItem& item);

				AbAttributeList Merge2New(const AbAttributeList& rhs) const;

				void Merge(const AbAttributeList& rhs);

				AbAttributeList Difference2New(const AbAttributeList& rhs) const;

				void Difference(const AbAttributeList& rhs);

				AbAttributeList operator+(const AbAttributeList& rhs) const;

				AbAttributeList operator-(const AbAttributeList& rhs) const;

				AbAttributeList& operator+=(const AbAttributeList& rhs);

				AbAttributeList& operator-=(const AbAttributeList& rhs);

				bool Search(const AbAttributeItem& item) const;

				size_t GetSerializedSize() const;

				size_t GetSize() const;

				const std::set<AbAttributeItem>& GetList() const
				{
					return m_list;
				}

				template<typename DestIt>
				DestIt Serialize(DestIt it) const
				{
					DestIt resIt = it;
					for (const auto& item : m_list)
					{
						resIt = item.Serialize(resIt);
					}
					return resIt;
				}

			private:
				std::set<AbAttributeItem> m_list;
			};
		}
	}
}
