#ifdef ENCLAVE_PLATFORM_SGX

#include "../DhtServer.h"

#include <cppcodec/base64_default_rfc4648.hpp>

#include <DecentApi/Common/Common.h>
#include <DecentApi/Common/make_unique.h>
#include <DecentApi/Common/Net/TlsCommLayer.h>
#include <DecentApi/Common/Ra/Crypto.h>
#include <DecentApi/Common/Ra/AppX509Cert.h>
#include <DecentApi/Common/Ra/TlsConfigAnyWhiteListed.h>
#include <DecentApi/Common/MbedTls/SessionTicketMgr.h>

#include <DecentApi/CommonEnclave/Net/EnclaveCntTranslator.h>
#include <DecentApi/CommonEnclave/Ra/TlsConfigSameEnclave.h>
#include <DecentApi/CommonEnclave/Tools/Crypto.h>
#include <DecentApi/CommonEnclave/Tools/DataSealer.h>

#include "../../../Common/Dht/FuncNums.h"
#include "../../../Common/Dht/TlsConfigAnyUser.h"

#include "../DhtStatesSingleton.h"

#ifdef DECENT_DHT_NAIVE_RA_VER
#include <DecentApi/Common/Ra/KeyContainer.h>
#include <DecentApi/Common/SGX/RaProcessorSp.h>
#include <DecentApi/CommonEnclave/SGX/RaProcessorClient.h>
#include <DecentApi/CommonEnclave/SGX/RaMutualCommLayer.h>

using namespace Decent::Sgx;
#endif // DECENT_DHT_NAIVE_RA_VER

using namespace Decent::Ra;
using namespace Decent::Dht;
using namespace Decent::Net;
using namespace Decent::Tools;
using namespace Decent::MbedTlsObj;

namespace
{
	DhtStates& gs_state = GetDhtStatesSingleton();

	static constexpr char gsk_ticketLabel[] = "TicketLabel";

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

#ifdef DECENT_DHT_NAIVE_RA_VER
	static RaProcessorSp::SgxQuoteVerifier quoteVrfy = [](const sgx_quote_t&)
	{
		return true;
	};
#endif // DECENT_DHT_NAIVE_RA_VER
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

		RaMutualCommLayer secComm(cnt,
			make_unique<RaProcessorClient>(gs_state.GetEnclaveId(),
				RaProcessorClient::sk_acceptAnyPubKey, RaProcessorClient::sk_acceptAnyRaConfig),
			make_unique<RaProcessorSp>(gs_state.GetIasConnector(),
				gs_state.GetKeyContainer().GetSignKeyPair(), gs_state.GetSpid(),
				RaProcessorSp::sk_defaultRpDataVrfy, quoteVrfy),
			[](const std::vector<uint8_t>& sessionBin) -> std::vector<uint8_t> //seal
		{
			return DataSealer::SealData(DataSealer::KeyPolicy::ByMrEnclave, gs_state, gsk_ticketLabel, std::vector<uint8_t>(), sessionBin, nullptr, 1024);
		},
			[](const std::vector<uint8_t>& ticket) -> std::vector<uint8_t> //unseal
		{
			std::vector<uint8_t> sessionBin;
			std::vector<uint8_t> meta;

			DataSealer::UnsealData(DataSealer::KeyPolicy::ByMrEnclave, gs_state, gsk_ticketLabel, ticket, meta, sessionBin, nullptr, 1024);

			return sessionBin;
		});

#else
		std::shared_ptr<TlsConfigSameEnclave> tlsCfg = std::make_shared<TlsConfigSameEnclave>(gs_state, TlsConfig::Mode::ServerVerifyPeer, GetDhtSessionTicketMgr());
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
		std::shared_ptr<TlsConfigSameEnclave> tlsCfg = std::make_shared<TlsConfigSameEnclave>(gs_state, TlsConfig::Mode::ServerVerifyPeer, GetDhtSessionTicketMgr());
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
		std::vector<uint8_t> appHash;

#ifdef DECENT_DHT_NAIVE_RA_VER

		std::unique_ptr<RaMutualCommLayer> tmpSecComm = make_unique<RaMutualCommLayer>(cnt,
			make_unique<RaProcessorClient>(gs_state.GetEnclaveId(),
				RaProcessorClient::sk_acceptAnyPubKey, RaProcessorClient::sk_acceptAnyRaConfig),
			make_unique<RaProcessorSp>(gs_state.GetIasConnector(),
				gs_state.GetKeyContainer().GetSignKeyPair(), gs_state.GetSpid(),
				RaProcessorSp::sk_defaultRpDataVrfy, quoteVrfy),
			[](const std::vector<uint8_t>& sessionBin) -> std::vector<uint8_t> //seal
		{
			return DataSealer::SealData(DataSealer::KeyPolicy::ByMrEnclave, gs_state, gsk_ticketLabel, std::vector<uint8_t>(), sessionBin, nullptr, 1024);
		},
			[](const std::vector<uint8_t>& ticket) -> std::vector<uint8_t> //unseal
		{
			std::vector<uint8_t> sessionBin;
			std::vector<uint8_t> meta;

			DataSealer::UnsealData(DataSealer::KeyPolicy::ByMrEnclave, gs_state, gsk_ticketLabel, ticket, meta, sessionBin, nullptr, 1024);

			return sessionBin;
		});

		const auto& mrEnclave = tmpSecComm->GetPeerIasReport().m_quote.report_body.mr_enclave.m;
		appHash = std::vector<uint8_t>(std::begin(mrEnclave), std::end(mrEnclave));

#else

		std::shared_ptr<TlsConfigAnyWhiteListed> tlsCfg = std::make_shared<TlsConfigAnyWhiteListed>(gs_state, TlsConfig::Mode::ServerVerifyPeer, GetAppSessionTicketMgr());
		std::unique_ptr<TlsCommLayer> tmpSecComm = make_unique<TlsCommLayer>(cnt, tlsCfg, true, nullptr);

		AppX509Cert appCert(tmpSecComm->GetPeerCertPem());
		std::string appHashStr = GetHashFromAppId(appCert.GetPlatformType(), appCert.GetAppId());
		appHash = cppcodec::base64_rfc4648::decode(appHashStr);

#endif

		std::unique_ptr<SecureCommLayer> secCnt = std::move(tmpSecComm);

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
		std::shared_ptr<TlsConfigAnyUser> tlsCfg = std::make_shared<TlsConfigAnyUser>(gs_state, TlsConfig::Mode::ServerVerifyPeer, GetAppSessionTicketMgr());
		std::unique_ptr<TlsCommLayer> tls = make_unique<TlsCommLayer>(cnt, tlsCfg, true, nullptr);

		return ProcessUserRequest(tls, cnt, GetSelfHash());
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
