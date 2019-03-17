#pragma once

#include <DecentApi/CommonApp/Net/SmartMessages.h>

namespace Decent
{
	namespace Dht
	{
		class FromDht : public Decent::Net::SmartMessages
		{
		public:
			static constexpr char const sk_ValueCat[] = "Dht::FromDht";

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

		class FromStore : public Decent::Net::SmartMessages
		{
		public:
			static constexpr char const sk_ValueCat[] = "Dht::FromStore";

		public:
			FromStore() :
				SmartMessages()
			{}

			FromStore(const Json::Value& msg) :
				SmartMessages(msg, sk_ValueCat)
			{}

			~FromStore()
			{}

			virtual std::string GetMessageCategoryStr() const override { return sk_ValueCat; }

		protected:
			virtual Json::Value& GetJsonMsg(Json::Value& outJson) const override;

		};

		class FromApp : public Decent::Net::SmartMessages
		{
		public:
			static constexpr char const sk_ValueCat[] = "Dht::FromApp";

		public:
			FromApp() :
				SmartMessages()
			{}

			FromApp(const Json::Value& msg) :
				SmartMessages(msg, sk_ValueCat)
			{}

			~FromApp()
			{}

			virtual std::string GetMessageCategoryStr() const override { return sk_ValueCat; }

		protected:
			virtual Json::Value& GetJsonMsg(Json::Value& outJson) const override;

		};
	}
}
