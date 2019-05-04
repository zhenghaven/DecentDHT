//#if ENCLAVE_PLATFORM_SGX

#include "../DhtServer.h"

#include <DecentApi/Common/Common.h>
#include <DecentApi/Common/Net/TlsCommLayer.h>
#include <DecentApi/Common/Ra/TlsConfigAnyWhiteListed.h>
#include <DecentApi/Common/MbedTls/SessionTicketMgr.h>

#include <DecentApi/CommonEnclave/Net/EnclaveCntTranslator.h>
#include <DecentApi/CommonEnclave/Ra/TlsConfigSameEnclave.h>

#include "../../../Common/Dht/FuncNums.h"

#include "../DhtConnectionPool.h"
#include "../DhtStatesSingleton.h"

using namespace Decent;
using namespace Decent::Dht;
using namespace Decent::Net;
using namespace Decent::MbedTlsObj;

namespace
{
	DhtStates& gs_state = Dht::GetDhtStatesSingleton();

	std::shared_ptr<SessionTicketMgr> GetDhtSessionTicketMgr()
	{
		static const std::shared_ptr<SessionTicketMgr> inst = std::make_shared<SessionTicketMgr>();
		return inst;
	}

	std::shared_ptr<SessionTicketMgr> GetAppSessionTicketMgr()
	{
		static const std::shared_ptr<SessionTicketMgr> inst = std::make_shared<SessionTicketMgr>();
		return inst;
	}
}

extern "C" int ecall_decent_dht_init(uint64_t self_addr, int is_first_node, uint64_t ex_addr)
{
	try
	{
		Init(self_addr, is_first_node, ex_addr);
	}
	catch (const std::exception& e)
	{
		PRINT_I("Failed to initialize DHT node. Error Message: %s.", e.what());
		return false;
	}
	return true;
}

extern "C" void ecall_decent_dht_deinit()
{
	try
	{
		DeInit();
	}
	catch (const std::exception& e)
	{
		PRINT_I("Failed to de-initialize DHT node. Error Message: %s.", e.what());
	}
}

extern "C" int ecall_decent_dht_proc_msg_from_dht(void* connection)
{
	if (!gs_state.GetDhtNode())
	{
		LOGW("Local DHT Node had not been initialized yet!");
		return false;
	}

	EnclaveCntTranslator cnt(connection);

	//LOGI("Processing message from DHT node...");

	try
	{
		std::shared_ptr<Ra::TlsConfigSameEnclave> tlsCfg = std::make_shared<Ra::TlsConfigSameEnclave>(gs_state, Ra::TlsConfig::Mode::ServerVerifyPeer, GetDhtSessionTicketMgr());
		Decent::Net::TlsCommLayer tls(cnt, tlsCfg, true);

		ProcessDhtQuery(tls);
	}
	catch (const std::exception& e)
	{
		PRINT_W("Failed to process message from DHT node.\nError message: %s.", e.what());
	}

	return false;
}

extern "C" int ecall_decent_dht_proc_msg_from_store(void* connection)
{
	if (!gs_state.GetDhtNode())
	{
		LOGW("Local DHT Node had not been initialized yet!");
		return false;
	}

	EnclaveCntTranslator cnt(connection);

	using namespace EncFunc::Store;

	//LOGI("Processing message from DHT store...");

	try
	{
		std::shared_ptr<Ra::TlsConfigSameEnclave> tlsCfg = std::make_shared<Ra::TlsConfigSameEnclave>(gs_state, Ra::TlsConfig::Mode::ServerVerifyPeer, GetDhtSessionTicketMgr());
		Decent::Net::TlsCommLayer tls(cnt, tlsCfg, true);

		ProcessStoreRequest(tls);
	}
	catch (const std::exception& e)
	{
		PRINT_W("Failed to process message from DHT store.\nError message: %s.", e.what());
	}

	return false;
}

extern "C" int ecall_decent_dht_proc_msg_from_app(void* connection)
{
	if (!gs_state.GetDhtNode())
	{
		LOGW("Local DHT Node had not been initialized yet!");
		return false;
	}

	EnclaveCntTranslator cnt(connection);

	using namespace EncFunc::App;

	LOGI("Processing message from App...");

	try
	{
		std::shared_ptr<Ra::TlsConfigAnyWhiteListed> tlsCfg = std::make_shared<Ra::TlsConfigAnyWhiteListed>(gs_state, Ra::TlsConfig::Mode::ServerVerifyPeer, GetAppSessionTicketMgr());
		Decent::Net::TlsCommLayer tls(cnt, tlsCfg, true);

		do
		{
			ProcessAppRequest(tls);
		} while (gs_state.GetAppConnectionPool().HoldInComingConnection(cnt, tls));
	}
	catch (const std::exception& e)
	{
		PRINT_W("Failed to process message from App.\nError message: %s.", e.what());
	}

	return false;
}

//#endif //ENCLAVE_PLATFORM_SGX
