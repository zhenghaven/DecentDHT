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
				AbAttributeList(std::vector<uint8_t>::const_iterator& it, const std::vector<uint8_t>::const_iterator& end);

				virtual ~AbAttributeList();

				void Insert(const AbAttributeItem& item);

				void Merge(const AbAttributeList& rhs);

				bool Search(const AbAttributeItem& item) const;

				size_t GetSerializedSize() const;

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