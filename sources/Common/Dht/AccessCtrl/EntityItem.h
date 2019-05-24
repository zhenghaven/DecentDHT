#pragma once

#include "ItemBase.h"

#include <vector>

#include <DecentApi/Common/general_key_types.h>

namespace Decent
{
	namespace Dht
	{
		namespace AccessCtrl
		{
			class EntityItem : public ItemBase
			{
			public: //static member:
				static constexpr size_t Size()
				{
					return sizeof(m_hash);
				}

			public:
				EntityItem() = delete;

				EntityItem(const general_256bit_hash& entityHash);

				/**
				* \brief	Constructor that parse Attribute item from binary array. Note: distance(it, end)
				* 			should >= Size().
				*
				* \param	it 	The iterator.
				* \param	end	The end.
				*/
				EntityItem(std::vector<uint8_t>::const_iterator& it, std::vector<uint8_t>::const_iterator end);

				EntityItem(const EntityItem& rhs);

				virtual ~EntityItem();

				EntityItem& operator=(const EntityItem& rhs);

				void Swap(EntityItem& other);

				const general_256bit_hash& GetEntityHash() const;

				virtual size_t GetSize() const override;

			protected:
				virtual const uint8_t* ByteBegin() const override;

				virtual const uint8_t* ByteEnd() const override;

			private:
				general_256bit_hash m_hash;
			};
		}
	}
}
