#ifdef ENCLAVE_PLATFORM_SGX

#include "../DhtServer.h"

#include <cppcodec/base64_default_rfc4648.hpp>

#include <DecentApi/Common/Common.h>
#include <DecentApi/Common/make_unique.h>
#include <DecentApi/Common/Net/TlsCommLayer.h>
#include <DecentApi/Common/Ra/Crypto.h>
#include <DecentApi/Common/Ra/TlsConfigAnyWhiteListed.h>
#include <DecentApi/Common/MbedTls/SessionTicketMgr.h>

#include <DecentApi/CommonEnclave/Net/EnclaveCntTranslator.h>
#include <DecentApi/CommonEnclave/Ra/TlsConfigSameEnclave.h>
#include <DecentApi/CommonEnclave/Tools/Crypto.h>

#include "../../../Common/Dht/FuncNums.h"
#include "../../../Common/Dht/TlsConfigAnyUser.h"

#include "../DhtStatesSingleton.h"

#ifndef DECENT_DHT_NAIVE_RA_VER
#	define DECENT_DHT_NAIVE_RA_VER
#endif // !DECENT_DHT_NAIVE_RA_VER

#ifdef DECENT_DHT_NAIVE_RA_VER
#include <DecentApi/Common/Ra/KeyContainer.h>
#include <DecentApi/Common/SGX/RaProcessorSp.h>
#include <DecentApi/Common/SGX/RaSpCommLayer.h>
#include <DecentApi/CommonEnclave/SGX/RaProcessorClient.h>
#include <DecentApi/CommonEnclave/SGX/RaClientCommLayer.h>
#endif // DECENT_DHT_NAIVE_RA_VER

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

extern "C" int ecall_decent_dht_init(uint64_t self_addr, int is_first_node, uint64_t ex_addr, size_t totalNode, size_t idx, void* ias_cntor, sgx_spid_t* spid, uint64_t enclave_Id)
{
	try
	{
		gs_state.SetIasConnector(ias_cntor);
		gs_state.SetEnclaveId(enclave_Id);
		gs_state.SetSpid(std::make_shared<sgx_spid_t>(*spid));

		Init(self_addr, is_first_node, ex_addr, totalNode, idx);
	}
	catch (const std::exception& e)
	{
		PRINT_I("Failed to initialize DHT node. Error msg: %s", e.what());
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
		PRINT_I("Failed to de-initialize DHT node. Error msg: %s", e.what());
	}
}

extern "C" int ecall_decent_dht_proc_msg_from_dht(void* connection, void** prev_held_cnt)
{
	if (!gs_state.GetDhtNode())
	{
		LOGW("Local DHT Node had not been initialized yet!");
		return false;
	}

	*prev_held_cnt = nullptr;
	EnclaveCntTranslator cnt(connection);

	//LOGI("Processing message from DHT node...");

	try
	{
#ifdef DECENT_DHT_NAIVE_RA_VER
		Sgx::RaProcessorSp::SgxQuoteVerifier quoteVrfy = [](const sgx_quote_t&)
		{
			return true;
		};

		std::unique_ptr<Sgx::RaProcessorSp> raProcSp = Tools::make_unique<Sgx::RaProcessorSp>(gs_state.GetIasConnector(),
			gs_state.GetKeyContainer().GetSignKeyPair(), gs_state.GetSpid(),
			Sgx::RaProcessorSp::sk_defaultRpDataVrfy, quoteVrfy);

		Sgx::RaSpCommLayer secComm(cnt, raProcSp);
#else
		std::shared_ptr<Ra::TlsConfigSameEnclave> tlsCfg = std::make_shared<Ra::TlsConfigSameEnclave>(gs_state, Ra::TlsConfig::Mode::ServerVerifyPeer, GetDhtSessionTicketMgr());
		Decent::Net::TlsCommLayer secComm(cnt, tlsCfg, true, nullptr);
#endif // DECENT_DHT_NAIVE_RA_VER

		ProcessDhtQuery(secComm, *prev_held_cnt);
	}
	catch (const std::exception& e)
	{
		PRINT_W("Failed to process message from DHT node. Error msg: %s", e.what());
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
		Decent::Net::TlsCommLayer tls(cnt, tlsCfg, true, nullptr);

		ProcessStoreRequest(tls);
	}
	catch (const std::exception& e)
	{
		PRINT_W("Failed to process message from DHT store. Error msg: %s", e.what());
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
		std::unique_ptr<TlsCommLayer> tls = Tools::make_unique<TlsCommLayer>(cnt, tlsCfg, true, nullptr);

		Ra::AppX509 appCert(tls->GetPeerCertPem());
		std::string appHashStr = Ra::GetHashFromAppId(appCert.GetPlatformType(), appCert.GetAppId());
		std::vector<uint8_t> appHash = cppcodec::base64_rfc4648::decode(appHashStr);

		std::unique_ptr<SecureCommLayer> secCnt = std::move(tls);

		return ProcessAppRequest(secCnt, cnt, appHash);
	}
	catch (const std::exception& e)
	{
		PRINT_W("Failed to process message from App. Error msg: %s", e.what());
		return false;
	}
}

extern "C" int ecall_decent_dht_proc_msg_from_user(void* connection)
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
		std::shared_ptr<Dht::TlsConfigAnyUser> tlsCfg = std::make_shared<Dht::TlsConfigAnyUser>(gs_state, Ra::TlsConfig::Mode::ServerVerifyPeer, GetAppSessionTicketMgr());
		std::unique_ptr<TlsCommLayer> tls = Tools::make_unique<TlsCommLayer>(cnt, tlsCfg, true, nullptr);

		return ProcessUserRequest(tls, cnt, Tools::GetSelfHash());
	}
	catch (const std::exception& e)
	{
		PRINT_W("Failed to process message from App. Error msg: %s", e.what());
		return false;
	}
}

extern "C" int ecall_decent_dht_forward_queue_worker()
{
	while (true)
	{
		try
		{
			QueryForwardWorker();
			return true;
		}
		catch (const std::exception& e)
		{
			PRINT_I("Query forward worker failed. Error Msg: %s", e.what());
		}
	}
}

extern "C" int ecall_decent_dht_reply_queue_worker()
{
	while (true)
	{
		try
		{
			QueryReplyWorker();
			return true;
		}
		catch (const std::exception& e)
		{
			PRINT_I("Query reply worker failed. Error Msg: %s", e.what());
		}
	}
}

extern "C" void ecall_decent_dht_terminate_workers()
{
	try
	{
		TerminateWorkers();
	}
	catch (const std::exception&)
	{}
}

#endif //ENCLAVE_PLATFORM_SGX
