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
#include <DecentApi/CommonApp/Ra/Messages.h>
#include <DecentApi/CommonApp/Tools/ConfigManager.h>

#include <DecentApi/Common/Common.h>
#include <DecentApi/Common/Ra/WhiteList/HardCoded.h>
#include <DecentApi/Common/MbedTls/BigNumber.h>

#include "../Common/Dht/AppNames.h"
#include "../Common_App/Tools.h"
#include "../Common_App/Dht/DecentDhtApp.h"
#include "../Common/Dht/CircularRange.h"
#include "../Common/Dht/LocalNode.h"
using namespace Decent;
using namespace Decent::Tools;
using namespace Decent::Dht;
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
	typedef LocalNode<MbedTlsObj::BigNumber, 32, uint64_t> BigNumLocalNode;

	std::shared_ptr<BigNumLocalNode::Pow2iArrayType > pow2iArray = std::make_shared<BigNumLocalNode::Pow2iArrayType >();
	for (size_t i = 0; i < pow2iArray->size(); ++i)
	{
		(*pow2iArray)[i].SetBit(i, true);
	}


	{
		static_assert(sizeof(FilledByteArray<32>::value) == 32, "The size of the filled array is unexpected. Probably the compiler doesn't support the implmentation.");
		MbedTlsObj::BigNumber smallest = 0;
		MbedTlsObj::BigNumber largest(FilledByteArray<32>::value, true);
		MbedTlsObj::BigNumber nodeId = 0;
		std::shared_ptr<BigNumLocalNode> locNode = std::make_shared<BigNumLocalNode>(nodeId, 0LL, smallest, largest, pow2iArray);

		locNode->FindSuccessor(MbedTlsObj::BigNumber(103LL));
	}

	std::cout << "================ Decent DHT ================" << std::endl;

	TCLAP::CmdLine cmd("Decent DHT", ' ', "ver", true);

	TCLAP::ValueArg<std::string> configPathArg("c", "config", "Path to the configuration file.", false, "Config.json", "String");
	TCLAP::ValueArg<std::string> wlKeyArg("w", "wl-key", "Key for the loaded whitelist.", false, "WhiteListKey", "String");
	TCLAP::SwitchArg isSendWlArg("s", "not-send-wl", "Do not send whitelist to Decent Server.", true);
	cmd.add(configPathArg);
	cmd.add(wlKeyArg);
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
	const ConfigItem& selfItem = configManager.GetItem(AppNames::sk_decentDHT);

	uint32_t serverIp = boost::asio::ip::address_v4::from_string(decentServerItem.GetAddr()).to_uint();
	std::unique_ptr<Net::Connection> serverCon;

	if (isSendWlArg.getValue())
	{
		serverCon = std::make_unique<Net::TCPConnection>(serverIp, decentServerItem.GetPort());
		serverCon->SendPack(LoadWhiteList(wlKeyArg.getValue(), configManager.GetLoadedWhiteListStr()));
	}

	serverCon = std::make_unique<Net::TCPConnection>(serverIp, decentServerItem.GetPort());

	std::shared_ptr<DecentDhtApp> enclave;
	try
	{
		enclave = std::make_shared<DecentDhtApp>(
			ENCLAVE_FILENAME, KnownFolderType::LocalAppDataEnclave, TOKEN_FILENAME, wlKeyArg.getValue(), *serverCon);
	}
	catch (const std::exception& e)
	{
		PRINT_W("Failed to start enclave program! Error Msg:\n%s", e.what());
		return -1;
	}

	Net::SmartServer smartServer;

	uint32_t selfIp = boost::asio::ip::address_v4::from_string(selfItem.GetAddr()).to_uint();
	std::unique_ptr<Net::Server> server(std::make_unique<Net::TCPServer>(selfIp, selfItem.GetPort()));

	smartServer.AddServer(server, enclave);
	smartServer.RunUtilUserTerminate();


	PRINT_I("Exit.");
	return 0;
}
