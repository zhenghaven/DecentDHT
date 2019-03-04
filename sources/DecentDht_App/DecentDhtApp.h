#pragma once

#include <DecentApi/DecentAppApp/DecentApp.h>

namespace Decent
{
	namespace Dht
	{
		class DecentDhtApp : public Decent::RaSgx::DecentApp
		{
		public:
			using DecentApp::DecentApp;

			virtual ~DecentDhtApp() {}

			virtual bool ProcessMsgForDhtLookup(Decent::Net::Connection& connection);

			virtual bool ProcessSmartMessage(const std::string& category, const Json::Value& jsonMsg, Decent::Net::Connection& connection) override;

		};
	}
}

