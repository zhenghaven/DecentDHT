#include "Messages.h"

#include <json/json.h>

#include <DecentApi/CommonApp/Net/MessageException.h>

using namespace Decent::Dht;

constexpr char const DhtLookup::sk_ValueCat[];

Json::Value& DhtLookup::GetJsonMsg(Json::Value& outJson) const
{
	Json::Value& root = SmartMessages::GetJsonMsg(outJson);

	root = Json::nullValue;

	return root;
}
