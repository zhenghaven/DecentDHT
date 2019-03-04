#pragma once

#include <DecentApi/CommonApp/Net/SmartMessages.h>

namespace Decent
{
	namespace Dht
	{
		class DhtLookup : public Decent::Net::SmartMessages
		{
		public:
			static constexpr char const sk_ValueCat[] = "DhtLookup";

		public:
			DhtLookup() :
				SmartMessages()
			{}

			DhtLookup(const Json::Value& msg) :
				SmartMessages(msg, sk_ValueCat)
			{}

			~DhtLookup()
			{}

			virtual std::string GetMessageCategoryStr() const override { return sk_ValueCat; }

		protected:
			virtual Json::Value& GetJsonMsg(Json::Value& outJson) const override;

		};
	}
}
