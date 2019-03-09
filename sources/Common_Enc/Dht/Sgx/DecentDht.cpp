#include "../DecentDht.h"

#include <DecentApi/Common/Common.h>
#include <DecentApi/Common/Ra/TlsConfig.h>
#include <DecentApi/Common/Net/TlsCommLayer.h>
#include <DecentApi/Common/MbedTls/BigNumber.h>

#include "../../../Common/Dht/FuncNums.h"
#include "../../../Common/Dht/AppNames.h"
#include "../../../Common/Dht/Node.h"

#include "../../../Common_Enc/Dht/DhtStatesSingleton.h"

using namespace Decent;
using namespace Decent::Dht;
using namespace Decent::MbedTlsObj;

namespace
{
	DhtStates& gs_state = Dht::GetDhtStatesSingleton();
}

static void FindSuccessor(void* connection, Decent::Net::TlsCommLayer &tls) 
{
	LOGI("Finding Successor...");
	std::array<uint8_t, DhtStates::sk_keySizeByte> keyBin{};
	tls.ReceiveRaw(connection, keyBin.data(), keyBin.size()); //2. Received queried ID

	ConstBigNumber queriedId(keyBin);
	LOGI("Recv queried ID: %s.", static_cast<const BigNumber&>(queriedId).ToBigEndianHexStr().c_str());
	
	DhtStates::DhtNodeType& localNode = *gs_state.GetDhtNode();

	DhtStates::DhtNodeType::NodeBaseType* resNode = localNode.FindSuccessor(queriedId);

	std::array<uint8_t, DhtStates::sk_keySizeByte> resKeyBin{};
	resNode->GetNodeId().ToBinary(resKeyBin);

	uint64_t resAddr = resNode->GetAddress();

	tls.SendRaw(connection, resKeyBin.data(), resKeyBin.size()); //3. Sent resultant ID
	tls.SendStruct(connection, resAddr); //4. Sent Address - Done!

	LOGI("Sent result ID: %s.", resNode->GetNodeId().ToBigEndianHexStr().c_str());
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

void Dht::ProcessDhtQueries(void * connection, Decent::Net::TlsCommLayer & tls)
{
	using namespace EncFunc::Dht;

	LOGI("Processing DHT queries...");

	NumType funcNum;
	tls.ReceiveStruct(connection, funcNum); //1. Received function type.

	switch (funcNum)
	{
	case k_findSuccessor:
		FindSuccessor(connection, tls);
		break;
	case k_findPredecessor:

		break;
	case k_getImmediateSuc:

		break;
	case k_getNodeId:

		break;
	default:
		break;
	}
}
