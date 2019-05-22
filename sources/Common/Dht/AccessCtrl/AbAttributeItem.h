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
			class AbAttributeItem : public ItemBase
			{
			public: //static member:
				static constexpr size_t Size()
				{
					return sizeof(m_hashes);
				}

				typedef general_256bit_hash BufferType[2];

			public:
				AbAttributeItem() = delete;

				AbAttributeItem(const general_256bit_hash& userHash, const general_256bit_hash& attrHash);

				/**
				 * \brief	Constructor that parse Attribute item from binary array. Note: distance(it, end)
				 * 			should >= Size().
				 *
				 * \param	it 	The iterator.
				 * \param	end	The end.
				 */
				AbAttributeItem(std::vector<uint8_t>::const_iterator& it, const std::vector<uint8_t>::const_iterator& end);

				AbAttributeItem(const AbAttributeItem& rhs);

				virtual ~AbAttributeItem();

				AbAttributeItem& operator=(const AbAttributeItem& rhs);

				void Swap(AbAttributeItem& other);

				const general_256bit_hash& GetUserHash() const;

				const general_256bit_hash& GetAttrHash() const;

				virtual size_t GetSize() const override;

			protected:
				virtual const uint8_t* ByteBegin() const override;

				virtual const uint8_t* ByteEnd() const override;

			private:
				BufferType m_hashes;
			};
		}
	}
}
