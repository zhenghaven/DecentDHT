#pragma once

#include <vector>

#include <DecentApi/Common/consttime_memequal.h>
#include <DecentApi/Common/general_key_types.h>

namespace Decent
{
	namespace Dht
	{
		namespace AccessCtrl
		{
			class AbAttributeItem
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

				bool operator==(const AbAttributeItem& rhs) const;

				bool operator>(const AbAttributeItem& rhs) const;

				bool operator>=(const AbAttributeItem& rhs) const;

				bool operator<(const AbAttributeItem& rhs) const;

				bool operator<=(const AbAttributeItem& rhs) const;

				template<typename OutputIt>
				void Serialize(OutputIt it) const
				{
					std::copy(ByteBegin(), ByteEnd(), it);
				}

			private:
				const uint8_t* ByteBegin() const;

				const uint8_t* ByteEnd() const;

				BufferType m_hashes;
			};
		}
	}
}
