#pragma once

#include <algorithm>

namespace Decent
{
	namespace Dht
	{
		namespace AccessCtrl
		{
			class ItemBase
			{
			public:
				ItemBase() = default;

				virtual ~ItemBase()
				{}

				virtual bool operator==(const ItemBase& rhs) const;

				virtual bool operator>(const ItemBase& rhs) const;

				virtual bool operator>=(const ItemBase& rhs) const;

				virtual bool operator<(const ItemBase& rhs) const;

				virtual bool operator<=(const ItemBase& rhs) const;

				template<typename DestIt>
				DestIt Serialize(DestIt it) const
				{
					return std::copy(ByteBegin(), ByteEnd(), it);
				}

				virtual size_t GetSize() const = 0;

			protected:
				virtual const uint8_t* ByteBegin() const = 0;

				virtual const uint8_t* ByteEnd() const = 0;
			};
		}
	}
}
