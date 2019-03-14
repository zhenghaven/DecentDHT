#include "../DhtServer.h"

#include <DecentApi/Common/Common.h>
#include <DecentApi/Common/Ra/TlsConfig.h>
#include <DecentApi/Common/Net/TlsCommLayer.h>

#include "../../../Common/Dht/AppNames.h"
#include "../DhtStatesSingleton.h"
#include "../EnclaveStore.h"
#include "../../../Common/Dht/FuncNums.h"


using namespace Decent;
using namespace Decent::Dht;
using namespace Decent::Net;
using namespace Decent::MbedTlsObj;

namespace
{
	DhtStates& gs_state = Dht::GetDhtStatesSingleton();
}


static void GetMigrateData(void * connection, Decent::Net::TlsCommLayer & tls){
    using namespace EncFunc::Store;

    std::array<uint8_t, DhtStates::sk_keySizeByte> keyBin{};
    tls.ReceiveRaw(keyBin.data(), keyBin.size());
    BigNumber start = BigNumber(keyBin);
    tls.ReceiveRaw(keyBin.data(), keyBin.size());
    BigNumber end = BigNumber(keyBin);


    gs_state.GetDhtStore().SendMigratingData([&tls](void* buffer, const size_t size) -> void
                                             {
                                                 tls.SendRaw(buffer, size);
                                             },
                                             [&tls](const BigNumber& key) -> void
                                             {
                                                 std::array<uint8_t, DhtStates::sk_keySizeByte> keyBuf{};
                                                 key.ToBinary(keyBuf);
                                                 tls.SendRaw(keyBuf.data(), keyBuf.size());
                                             },start, end);

}

static void SetMigrateData(Decent::Net::TlsCommLayer & tls){
    using namespace EncFunc::Store;

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

static void SetData(Decent::Net::TlsCommLayer & tls){

    using namespace EncFunc::Store;

    std::array<uint8_t, DhtStates::sk_keySizeByte> keyBin{};
    tls.ReceiveRaw(keyBin.data(), keyBin.size());
    BigNumber key = BigNumber(keyBin);

    std::string buffer;
    tls.ReceiveMsg(buffer);


//    gs_state.GetDhtStore().SetValue(key, buffer);

}


static void GetData(Decent::Net::TlsCommLayer & tls){

    std::array<uint8_t, DhtStates::sk_keySizeByte> keyBin{};
    tls.ReceiveRaw(keyBin.data(), keyBin.size());
    BigNumber key = BigNumber(keyBin);

    std::string buffer;


//    gs_state.GetDhtStore().GetValue(key, buffer);

    tls.SendMsg(buffer);

}




extern "C" int ecall_decent_dht_proc_msg_from_dht(void* connection)
{
	if (!gs_state.GetDhtNode())
	{
		LOGW("Local DHT Node had not been initialized yet!");
		return false;
	}

	std::shared_ptr<Decent::Ra::TlsConfig> tlsCfg = std::make_shared<Decent::Ra::TlsConfig>(AppNames::sk_decentDHT, gs_state, true);
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

	std::shared_ptr<Decent::Ra::TlsConfig> tlsCfg = std::make_shared<Decent::Ra::TlsConfig>(AppNames::sk_decentDHT, gs_state, true);
	Decent::Net::TlsCommLayer tls(connection, tlsCfg, true);

    LOGI("Processing DHT queries...");

    NumType funcNum;
    tls.ReceiveStruct(connection, funcNum); //1. Received function type.

    switch (funcNum)
    {

        case k_getMigrateData:
            GetMigrateData(connection, tls);
            break;

        case k_setMigrateData:
            SetMigrateData(tls);
            break;

        case k_getData:
            GetData(tls);
            break;

        case k_setData:
            SetData(tls);
            break;

        default:
            break;
    }


	//gs_state.GetDhtStore().RecvMigratingData()

	return false;
}
