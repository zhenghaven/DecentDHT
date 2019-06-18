#include <string>
#include <memory>
#include <iostream>

#include <tclap/CmdLine.h>
#include <boost/filesystem.hpp>

#include <DecentApi/CommonApp/Common.h>

#include <DecentApi/CommonApp/Net/SmartServer.h>
#include <DecentApi/CommonApp/Net/TCPServer.h>
#include <DecentApi/CommonApp/Net/TCPConnection.h>
#include <DecentApi/CommonApp/Tools/DiskFile.h>
#include <DecentApi/CommonApp/Tools/FileSystemUtil.h>
#include <DecentApi/CommonApp/Threading/MainThreadAsynWorker.h>

#include <DecentApi/Common/Common.h>
#include <DecentApi/Common/Ra/RequestCategory.h>
#include <DecentApi/Common/Ra/WhiteList/WhiteList.h>
#include <DecentApi/Common/Net/CntPoolConnection.h>
#include <DecentApi/Common/MbedTls/BigNumber.h>

#include <DecentApi/DecentAppApp/DecentAppConfig.h>

#include "../Common/Dht/AppName.h"
#include "../Common/Dht/RequestCategory.h"

#include "../Common_App/Dht/SGX/DecentDhtApp.h"
#include "../Common_App/Dht/DhtConnectionPool.h"

using namespace Decent;
using namespace Decent::Tools;
using namespace Decent::Dht;
using namespace Decent::Net;
using namespace Decent::Threading;
using namespace Decent::AppConfig;

static std::shared_ptr<DhtConnectionPool> GetTcpConnectionPool()
{
	static std::shared_ptr<DhtConnectionPool> tcpConnectionPool = std::make_shared<DhtConnectionPool>(1000, 1000);
	return tcpConnectionPool;
}

extern "C" void* ocall_decent_dht_cnt_mgr_get_dht(uint64_t address)
{
	try
	{
		std::unique_ptr<ConnectionBase> cnt = GetTcpConnectionPool()->Get(address);
		cnt->SendPack(RequestCategory::sk_fromDht);

		return new CntPoolConnection<uint64_t>(address, std::move(cnt), GetTcpConnectionPool());
	}
	catch (const std::exception&)
	{
		return nullptr;
	}
}

extern "C" void* ocall_decent_dht_cnt_mgr_get_store(uint64_t address)
{
	try
	{
		std::unique_ptr<ConnectionBase> cnt = GetTcpConnectionPool()->Get(address);
		cnt->SendPack(RequestCategory::sk_fromStore);

		return new CntPoolConnection<uint64_t>(address, std::move(cnt), GetTcpConnectionPool());
	}
	catch (const std::exception&)
	{
		return nullptr;
	}
}

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
	//------- Construct main thread worker at very first:
	std::shared_ptr<MainThreadAsynWorker> mainThreadWorker = std::make_shared<MainThreadAsynWorker>();

	std::cout << "================ Decent DHT ================" << std::endl;

	//------- Setup Smart Server:
	Net::SmartServer smartServer(mainThreadWorker);

	//------- Setup command line argument parser:
	TCLAP::CmdLine cmd("Decent DHT", ' ', "ver", true);

	TCLAP::ValueArg<std::string> configPathArg("c", "config", "Path to the configuration file.", false, "Config.json", "String");
	TCLAP::ValueArg<std::string> wlKeyArg("w", "wl-key", "Key for the loaded whitelist.", false, "WhiteListKey", "String");
	TCLAP::ValueArg<int> selfNodePortNum("s", "self-port", "Port number for existing node.", false, 0, "[0-65535]");
	TCLAP::ValueArg<int> exNodePortNum("p", "ex-port", "Port number for existing node.", false, 0, "[0-65535]");
	TCLAP::SwitchArg isSendWlArg("n", "not-send-wl", "Do not send whitelist to Decent Server.", true);
	TCLAP::ValueArg<int> totalNode("t", "total-node", "Total number of nodes in the network.", true, 0, "[0-MAX_INT]");
	TCLAP::ValueArg<int> nodeIdx("i", "node-idx", "The index of the current adding node.", true, 0, "[0-MAX_INT]");
	cmd.add(configPathArg);
	cmd.add(wlKeyArg);
	cmd.add(selfNodePortNum);
	cmd.add(exNodePortNum);
	cmd.add(isSendWlArg);
	cmd.add(totalNode);
	cmd.add(nodeIdx);

	cmd.parse(argc, argv);

	//------- Read configuration file:
	std::unique_ptr<DecentAppConfig> configMgr;
	try
	{
		std::string configJsonStr;
		DiskFile file(configPathArg.getValue(), FileBase::Mode::Read, true);
		configJsonStr.resize(file.GetFileSize());
		file.ReadBlockExactSize(configJsonStr);

		configMgr = std::make_unique<DecentAppConfig>(configJsonStr);
	}
	catch (const std::exception& e)
	{
		PRINT_W("Failed to load configuration file. Error Msg: %s", e.what());
		return -1;
	}

	//------- Read Decent Server Configuration:
	uint32_t serverIp = 0;
	uint16_t serverPort = 0;
	try
	{
		const auto& decentServerConfig = configMgr->GetEnclaveList().GetItem(Ra::WhiteList::sk_nameDecentServer);

		serverIp = Net::TCPConnection::GetIpAddressFromStr(decentServerConfig.GetAddr());
		serverPort = decentServerConfig.GetPort();
	}
	catch (const std::exception& e)
	{
		PRINT_W("Failed to read Decent Server configuration. Error Msg: %s", e.what());
		return -1;
	}

	//------- Read self configuration:
	uint32_t selfIp = 0;
	uint16_t selfPort = 0;
	uint64_t selfFullAddr = 0;
	uint64_t exNodeFullAddr = 0;
	try
	{
		const auto& selfItem = configMgr->GetEnclaveList().GetItem(AppName::sk_decentDht);

		selfIp = TCPConnection::GetIpAddressFromStr(selfItem.GetAddr());
		selfPort = selfNodePortNum.getValue() ? selfNodePortNum.getValue() : selfItem.GetPort();

		selfFullAddr = TCPConnection::CombineIpAndPort(selfIp, selfPort);
		exNodeFullAddr = exNodePortNum.getValue() == 0 ? 0 : TCPConnection::CombineIpAndPort(selfIp, static_cast<uint16_t>(exNodePortNum.getValue()));
	}
	catch (const std::exception& e)
	{
		PRINT_W("Failed to read self configuration. Error Msg: %s", e.what());
		return -1;
	}

	std::unique_ptr<ConnectionBase> serverCon;
	//------- Send loaded white list to Decent Server when needed.
	try
	{
		if (isSendWlArg.getValue())
		{
			serverCon = std::make_unique<TCPConnection>(serverIp, serverPort);
			serverCon->SendPack(Ra::RequestCategory::sk_loadWhiteList);
			serverCon->SendPack(wlKeyArg.getValue());
			serverCon->SendPack(configMgr->GetEnclaveList().GetLoadedWhiteListStr());
			char ackMsg[] = "ACK";
			serverCon->ReceiveRawGuarantee(&ackMsg, sizeof(ackMsg));
		}
	}
	catch (const std::exception& e)
	{
		PRINT_W("Failed to send loaded white list to Decent Server. Error Msg: %s", e.what());
		return -1;
	}

	//------- Setup TCP server:
	std::unique_ptr<Server> server;
	try
	{
		server = std::make_unique<Net::TCPServer>(selfIp, selfPort);
	}
	catch (const std::exception& e)
	{
		PRINT_W("Failed to start TCP server. Error Message: %s", e.what());
		return -1;
	}

	//------- Setup Enclave:
	std::shared_ptr<DecentDhtApp> enclave;
	try
	{
		serverCon = std::make_unique<TCPConnection>(serverIp, serverPort);

		boost::filesystem::path tokenPath = GetKnownFolderPath(KnownFolderType::LocalAppDataEnclave).append(TOKEN_FILENAME);

		enclave = std::make_shared<DecentDhtApp>(
			ENCLAVE_FILENAME, tokenPath, wlKeyArg.getValue(), *serverCon);

		smartServer.AddServer(server, enclave, GetTcpConnectionPool(), 1, 1002);

		enclave->InitDhtNode(selfFullAddr, exNodeFullAddr, totalNode.getValue(), nodeIdx.getValue());

		enclave->InitQueryWorkers(1, 1);
	}
	catch (const std::exception& e)
	{
		PRINT_W("Failed to start enclave program. Error Msg: %s", e.what());
		return -1;
	}

	//------- keep running until an interrupt signal (Ctrl + C) is received.
	mainThreadWorker->UpdateUntilInterrupt();

	//------- Exit...
	enclave.reset();
	smartServer.Terminate();

	PRINT_I("Exit.");
	return 0;
}
