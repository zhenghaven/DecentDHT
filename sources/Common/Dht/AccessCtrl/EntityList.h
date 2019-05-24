#pragma once

#include <set>
#include <vector>

#include "EntityItem.h"

namespace Decent
{
	namespace Dht
	{
		namespace AccessCtrl
		{
			class EntityList
			{
			public:
				EntityList();

				/**
				* \brief	Constructor that parse Attribute item from binary array. Note: distance(it, end)
				* 			should be exact size of (N * AbAttributeItem::Size()).
				*
				* \param	it 	The iterator.
				* \param	end	The end.
				*/
				EntityList(std::vector<uint8_t>::const_iterator& it, std::vector<uint8_t>::const_iterator end);

				virtual ~EntityList();

				void Insert(const EntityItem& item);

				void Merge(const EntityList& rhs);

				bool Search(const EntityItem& item) const;

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
				std::set<EntityItem> m_list;
			};
		}
	}
}
