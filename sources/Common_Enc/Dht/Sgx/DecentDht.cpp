#include "../DhtServer.h"

#include <DecentApi/Common/Common.h>
#include <DecentApi/Common/Net/TlsCommLayer.h>
#include <DecentApi/Common/Ra/TlsConfigAnyWhiteListed.h>
#include <DecentApi/CommonEnclave/Ra/TlsConfigSameEnclave.h>

#include "../../../Common/Dht/AppNames.h"
#include "../../../Common/Dht/FuncNums.h"
#include "../DhtStatesSingleton.h"
#include "../EnclaveStore.h"

using namespace Decent;
using namespace Decent::Dht;
using namespace Decent::Net;
using namespace Decent::MbedTlsObj;

namespace
{
	DhtStates& gs_state = Dht::GetDhtStatesSingleton();
}

static void GetMigrateData(Decent::Net::TlsCommLayer & tls)
{
    std::array<uint8_t, DhtStates::sk_keySizeByte> startKeyBin{};
    tls.ReceiveRaw(startKeyBin.data(), startKeyBin.size());
	ConstBigNumber start(startKeyBin);

	std::array<uint8_t, DhtStates::sk_keySizeByte> endKeyBin{};
    tls.ReceiveRaw(endKeyBin.data(), endKeyBin.size());
	ConstBigNumber end(endKeyBin);

	gs_state.GetDhtStore().SendMigratingData(
		[&tls](void* buffer, const size_t size) -> void
	{
		tls.SendRaw(buffer, size);
	},
		[&tls](const BigNumber& key) -> void
	{
		std::array<uint8_t, DhtStates::sk_keySizeByte> keyBuf{};
		key.ToBinary(keyBuf);
		tls.SendRaw(keyBuf.data(), keyBuf.size());
	},
		start, end);
}

static void SetMigrateData(Decent::Net::TlsCommLayer & tls)
{
	gs_state.GetDhtStore().RecvMigratingData(
		[&tls](void* buffer, const size_t size) -> void
	{
		tls.ReceiveRaw(buffer, size);
	},
		[&tls]() -> BigNumber
	{
		std::array<uint8_t, DhtStates::sk_keySizeByte> keyBuf{};
		tls.ReceiveRaw(keyBuf.data(), keyBuf.size());
		return BigNumber(keyBuf);
	});

}

static void SetData(Decent::Net::TlsCommLayer & tls)
{
	std::array<uint8_t, DhtStates::sk_keySizeByte> keyBin{};
	tls.ReceiveRaw(keyBin.data(), keyBin.size());
	ConstBigNumber key(keyBin);

	std::vector<uint8_t> buffer;
	tls.ReceiveMsg(buffer);

	gs_state.GetDhtStore().SetValue(key, std::move(buffer));
}


static void GetData(Decent::Net::TlsCommLayer & tls)
{
	std::array<uint8_t, DhtStates::sk_keySizeByte> keyBin{};
	tls.ReceiveRaw(keyBin.data(), keyBin.size());
	BigNumber key = BigNumber(keyBin);

	std::vector<uint8_t> buffer;
	gs_state.GetDhtStore().GetValue(key, buffer);

	tls.SendMsg(buffer);
}

extern "C" int ecall_decent_dht_proc_msg_from_dht(void* connection)
{
	if (!gs_state.GetDhtNode())
	{
		LOGW("Local DHT Node had not been initialized yet!");
		return false;
	}

	std::shared_ptr<Ra::TlsConfigSameEnclave> tlsCfg = std::make_shared<Ra::TlsConfigSameEnclave>(gs_state, Ra::TlsConfig::Mode::ServerVerifyPeer);
	Decent::Net::TlsCommLayer tls(connection, tlsCfg, true);

	ProcessDhtQueries(connection, tls);

	return false;
}

extern "C" int ecall_decent_dht_proc_msg_from_store(void* connection)
{
	if (!gs_state.GetDhtNode())
	{
		LOGW("Local DHT Node had not been initialized yet!");
		return false;
	}

	using namespace EncFunc::Store;

	std::shared_ptr<Ra::TlsConfigSameEnclave> tlsCfg = std::make_shared<Ra::TlsConfigSameEnclave>(gs_state, Ra::TlsConfig::Mode::ServerVerifyPeer);
	Decent::Net::TlsCommLayer tls(connection, tlsCfg, true);

	LOGI("Processing DHT store operations...");

	NumType funcNum;
	tls.ReceiveStruct(funcNum); //1. Received function type.

	switch (funcNum)
	{
	case k_getMigrateData:
		GetMigrateData(tls);
		break;

	case k_setMigrateData:
		SetMigrateData(tls);
		break;

	default:
		break;
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

	using namespace EncFunc::Store;

	std::shared_ptr<Ra::TlsConfigAnyWhiteListed> tlsCfg = std::make_shared<Ra::TlsConfigAnyWhiteListed>(gs_state, Ra::TlsConfig::Mode::ServerVerifyPeer);
	Decent::Net::TlsCommLayer tls(connection, tlsCfg, true);

	LOGI("Processing DHT store operations...");

	NumType funcNum;
	tls.ReceiveStruct(funcNum); //1. Received function type.

	switch (funcNum)
	{
	case k_getData:
		GetData(tls);
		break;

	case k_setData:
		SetData(tls);
		break;

	default:
		break;
	}

	return false;
}
