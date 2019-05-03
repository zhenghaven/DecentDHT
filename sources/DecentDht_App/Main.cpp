#include <string>
#include <memory>
#include <iostream>

#include <tclap/CmdLine.h>
#include <boost/asio/ip/address_v4.hpp>
#include <sgx_quote.h>

#include <DecentApi/CommonApp/Common.h>

#include <DecentApi/CommonApp/Net/SmartMessages.h>
#include <DecentApi/CommonApp/Net/TCPConnection.h>
#include <DecentApi/CommonApp/Net/TCPServer.h>
#include <DecentApi/CommonApp/Net/SmartServer.h>
#include <DecentApi/CommonApp/Threading/MainThreadAsynWorker.h>
#include <DecentApi/CommonApp/Ra/Messages.h>
#include <DecentApi/CommonApp/Tools/ConfigManager.h>
#include <DecentApi/CommonApp/Tools/FileSystemUtil.h>

#include <DecentApi/Common/Common.h>
#include <DecentApi/Common/Ra/WhiteList/WhiteList.h>
#include <DecentApi/Common/MbedTls/BigNumber.h>

#include "../Common_App/Tools.h"
#include "../Common_App/Dht/DecentDhtApp.h"

using namespace Decent;
using namespace Decent::Tools;
using namespace Decent::Dht;
using namespace Decent::Net;
using namespace Decent::Threading;
using namespace Decent::Ra::Message;

/**
 * \brief	Main entry-point for this application
 *
 * \param	argc	The number of command-line arguments provided.
 * \param	argv	An array of command-line argument strings.
 *
 * \return	Exit-code for the process - 0 for success, else an error code.
 */
int main(int argc, char ** argv)
{
	std::cout << "================ Decent DHT ================" << std::endl;

	TCLAP::CmdLine cmd("Decent DHT", ' ', "ver", true);

	TCLAP::ValueArg<std::string> configPathArg("c", "config", "Path to the configuration file.", false, "Config.json", "String");
	TCLAP::ValueArg<std::string> wlKeyArg("w", "wl-key", "Key for the loaded whitelist.", false, "WhiteListKey", "String");
	TCLAP::ValueArg<int> selfNodePortNum("s", "self-port", "Port number for existing node.", false, 0, "[0-65535]");
	TCLAP::ValueArg<int> exNodePortNum("p", "ex-port", "Port number for existing node.", false, 0, "[0-65535]");
	TCLAP::SwitchArg isSendWlArg("n", "not-send-wl", "Do not send whitelist to Decent Server.", true);
	cmd.add(configPathArg);
	cmd.add(wlKeyArg);
	cmd.add(selfNodePortNum);
	cmd.add(exNodePortNum);
	cmd.add(isSendWlArg);

	cmd.parse(argc, argv);

	std::string configJsonStr;
	if (!Decent::Dht::Tools::GetConfigurationJsonString(configPathArg.getValue(), configJsonStr))
	{
		PRINT_W("Failed to load configuration file.");
		return -1;
	}
	ConfigManager configManager(configJsonStr);

	const ConfigItem& decentServerItem = configManager.GetItem(Ra::WhiteList::sk_nameDecentServer);
	const ConfigItem& selfItem = configManager.GetItem("DecentDHT");

	uint32_t serverIp = boost::asio::ip::address_v4::from_string(decentServerItem.GetAddr()).to_uint();
	std::unique_ptr<Connection> serverCon;

	if (isSendWlArg.getValue())
	{
		serverCon = std::make_unique<TCPConnection>(serverIp, decentServerItem.GetPort());
		serverCon->SendSmartMsg(LoadWhiteList(wlKeyArg.getValue(), configManager.GetLoadedWhiteListStr()));
	}

	serverCon = std::make_unique<TCPConnection>(serverIp, decentServerItem.GetPort());

	uint32_t selfIp = TCPConnection::GetIpAddressFromStr(selfItem.GetAddr());
	uint64_t selfFullAddr = TCPConnection::CombineIpAndPort(selfIp, selfNodePortNum.getValue() ? selfNodePortNum.getValue() : selfItem.GetPort());
	uint64_t exNodeFullAddr = exNodePortNum.getValue() == 0 ? 0 : TCPConnection::CombineIpAndPort(selfIp, static_cast<uint16_t>(exNodePortNum.getValue()));

	MainThreadAsynWorker mainThreadWorker;
	SmartServer smartServer(2, mainThreadWorker);
	std::unique_ptr<Server> server(std::make_unique<TCPServer>(selfIp, selfNodePortNum.getValue() ? selfNodePortNum.getValue() : selfItem.GetPort()));

	std::shared_ptr<DecentDhtApp> enclave;
	try
	{
		boost::filesystem::path tokenPath = GetKnownFolderPath(KnownFolderType::LocalAppDataEnclave).append(TOKEN_FILENAME);
		enclave = std::make_shared<DecentDhtApp>(
			ENCLAVE_FILENAME, tokenPath, wlKeyArg.getValue(), *serverCon);

		smartServer.AddServer(server, enclave);

		enclave->InitDhtNode(selfFullAddr, exNodeFullAddr);
	}
	catch (const std::exception& e)
	{
		PRINT_W("Failed to start enclave program! Error Msg:\n%s", e.what());
		return -1;
	}

	mainThreadWorker.UpdateUntilInterrupt();


	PRINT_I("Exit.");
	return 0;
}
