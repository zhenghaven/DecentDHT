#pragma once

#include <DecentApi/CommonApp/Net/SmartMessages.h>

namespace Decent
{
	namespace Dht
	{
		class FromDht : public Decent::Net::SmartMessages
		{
		public:
			static constexpr char const sk_ValueCat[] = "FromDht";

		public:
			FromDht() :
				SmartMessages()
			{}

			FromDht(const Json::Value& msg) :
				SmartMessages(msg, sk_ValueCat)
			{}

			~FromDht()
			{}

			virtual std::string GetMessageCategoryStr() const override { return sk_ValueCat; }

		protected:
			virtual Json::Value& GetJsonMsg(Json::Value& outJson) const override;

		};
	}
}
