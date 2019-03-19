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

			virtual ~DecentDhtApp();

			virtual bool ProcessMsgFromDht(Decent::Net::Connection& connection);

			virtual bool ProcessMsgFromStore(Decent::Net::Connection& connection);

			virtual bool ProcessMsgFromApp(Decent::Net::Connection& connection);

			virtual bool ProcessSmartMessage(const std::string& category, const Json::Value& jsonMsg, Decent::Net::Connection& connection) override;

			void InitDhtNode(uint64_t selfAddr, uint64_t exNodeAddr);

		private:
		};
	}
}

