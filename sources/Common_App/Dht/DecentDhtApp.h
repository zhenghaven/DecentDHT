#pragma once

#include <DecentApi/DecentAppApp/DecentApp.h>

namespace Decent
{
	namespace Dht
	{
		class DecentDhtApp : public Decent::RaSgx::DecentApp
		{
		public:
			DecentDhtApp(const std::string& enclavePath, const std::string& tokenPath, const std::string& wListKey, Net::Connection& serverConn, uint64_t selfAddr, uint64_t exNodeAddr);
			DecentDhtApp(const fs::path& enclavePath, const fs::path& tokenPath, const std::string& wListKey, Net::Connection& serverConn, uint64_t selfAddr, uint64_t exNodeAddr);
			DecentDhtApp(const std::string& enclavePath, const Decent::Tools::KnownFolderType tokenLocType, const std::string& tokenFileName, const std::string& wListKey, Net::Connection& serverConn, uint64_t selfAddr, uint64_t exNodeAddr);

			virtual ~DecentDhtApp();

			virtual bool ProcessMsgFromDht(Decent::Net::Connection& connection);

			virtual bool ProcessMsgFromStore(Decent::Net::Connection& connection);

			virtual bool ProcessMsgFromApp(Decent::Net::Connection& connection);

			virtual bool ProcessSmartMessage(const std::string& category, const Json::Value& jsonMsg, Decent::Net::Connection& connection) override;

		private:
			void InitEnclave(uint64_t selfAddr, uint64_t exNodeAddr);
		};
	}
}

