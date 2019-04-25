#include "DhtServer.h"

#include <DecentApi/Common/Common.h>
#include <DecentApi/Common/Ra/TlsConfig.h>
#include <DecentApi/Common/Net/TlsCommLayer.h>
#include <DecentApi/Common/MbedTls/BigNumber.h>

#include "../../Common/Dht/LocalNode.h"
#include "../../Common/Dht/FuncNums.h"
#include "../../Common/Dht/NodeBase.h"

#include "EnclaveStore.h"
#include "NodeConnector.h"
#include "DhtStatesSingleton.h"

using namespace Decent;
using namespace Decent::Dht;
using namespace Decent::MbedTlsObj;

namespace
{
	DhtStates& gs_state = Dht::GetDhtStatesSingleton();

	static char gsk_ack[] = "ACK";
}

void Dht::DeUpdateFingerTable(Decent::Net::TlsCommLayer &tls)
{
	//LOGI("DHT Server: De-Updating FingerTable...");
	std::array<uint8_t, DhtStates::sk_keySizeByte> oldIdBin{};
	uint64_t i;

	tls.ReceiveRaw(oldIdBin.data(), oldIdBin.size()); //2. Receive old ID.

	DhtStates::DhtLocalNodeType::NodeBasePtr s = NodeConnector::ReceiveNode(tls); //3. Receive node.
	tls.ReceiveStruct(i); //3. Receive i. - Done!
	PRINT_I("De-updating finger table; i = %llu.", i);

	DhtStates::DhtLocalNodePtrType localNode = gs_state.GetDhtNode();

	localNode->DeUpdateFingerTable(ConstBigNumber(oldIdBin), s, i);

	tls.SendStruct(gsk_ack);
	//LOGI("");
}

void Dht::UpdateFingerTable(Decent::Net::TlsCommLayer &tls)
{
	//LOGI("DHT Server: Updating FingerTable...");
	DhtStates::DhtLocalNodeType::NodeBasePtr s = NodeConnector::ReceiveNode(tls); //2. Receive node.

	uint64_t i;
	tls.ReceiveStruct(i); //3. Receive i. - Done!
	PRINT_I("Updating finger table; i = %llu.", i);

	DhtStates::DhtLocalNodePtrType localNode = gs_state.GetDhtNode();

	localNode->UpdateFingerTable(s, i);

	tls.SendStruct(gsk_ack);
	//LOGI("");
}

void Dht::GetImmediatePredecessor(Decent::Net::TlsCommLayer &tls)
{
	//LOGI("DHT Server: Getting Immediate Predecessor...");

	DhtStates::DhtLocalNodePtrType localNode = gs_state.GetDhtNode();

	NodeConnector::SendNode(tls, localNode->GetImmediatePredecessor()); //2. Send Node. - Done!
	//LOGI("");
}

void Dht::SetImmediatePredecessor(Decent::Net::TlsCommLayer &tls)
{
	//LOGI("DHT Server: Setting Immediate Predecessor...");

	DhtStates::DhtLocalNodePtrType localNode = gs_state.GetDhtNode();

	localNode->SetImmediatePredecessor(NodeConnector::ReceiveNode(tls)); //2. Receive Node. - Done!

	tls.SendStruct(gsk_ack);
	//LOGI("");
}

void Dht::GetImmediateSucessor(Decent::Net::TlsCommLayer &tls)
{
    //LOGI("DHT Server: Finding Immediate Successor...");

    DhtStates::DhtLocalNodePtrType localNode = gs_state.GetDhtNode();

	NodeConnector::SendNode(tls, localNode->GetImmediateSuccessor()); //2. Send Node. - Done!
	//LOGI("");
}

void Dht::GetNodeId(Decent::Net::TlsCommLayer &tls)
{
    //LOGI("DHT Server: Getting the NodeId...");
	DhtStates::DhtLocalNodePtrType localNode = gs_state.GetDhtNode();
    std::array<uint8_t, DhtStates::sk_keySizeByte> keyBin{};
    localNode->GetNodeId().ToBinary(keyBin);

    tls.SendRaw(keyBin.data(), keyBin.size()); //2. Sent nodeId ID

    //LOGI("Sent result ID: %s.", localNode->GetNodeId().ToBigEndianHexStr().c_str());
	//LOGI("");
}

void Dht::FindPredecessor(Decent::Net::TlsCommLayer &tls)
{
    //LOGI("DHT Server: Finding Predecessor...");
    std::array<uint8_t, DhtStates::sk_keySizeByte> keyBin{};
    tls.ReceiveRaw(keyBin.data(), keyBin.size()); //2. Received queried ID

    ConstBigNumber queriedId(keyBin);
    //LOGI("Recv queried ID: %s.", static_cast<const BigNumber&>(queriedId).ToBigEndianHexStr().c_str());

	DhtStates::DhtLocalNodePtrType localNode = gs_state.GetDhtNode();

	NodeConnector::SendNode(tls, localNode->FindPredecessor(queriedId)); //3. Send Node. - Done!
	//LOGI("");
}

void Dht::FindSuccessor(Decent::Net::TlsCommLayer &tls)
{
	//LOGI("DHT Server: Finding Successor...");
	std::array<uint8_t, DhtStates::sk_keySizeByte> keyBin{};
	tls.ReceiveRaw(keyBin.data(), keyBin.size()); //2. Received queried ID

	ConstBigNumber queriedId(keyBin);
	//LOGI("Recv queried ID: %s.", static_cast<const BigNumber&>(queriedId).ToBigEndianHexStr().c_str());

	DhtStates::DhtLocalNodePtrType localNode = gs_state.GetDhtNode();

	NodeConnector::SendNode(tls, localNode->FindSuccessor(queriedId)); //3. Send Node. - Done!
	//LOGI("");
}

void Dht::ProcessDhtQuery(Decent::Net::TlsCommLayer & tls)
{
	using namespace EncFunc::Dht;

	//LOGI("DHT Server: Processing DHT queries...");

	NumType funcNum;
	tls.ReceiveStruct(funcNum); //1. Received function type.

	switch (funcNum)
	{

	case k_findSuccessor:
		FindSuccessor(tls);
		break;

	case k_findPredecessor:
        FindPredecessor(tls);
		break;

	case k_getImmediateSuc:
        GetImmediateSucessor(tls);
		break;

	case k_getNodeId:
	    GetNodeId(tls);
		break;

	case k_setImmediatePre:
		SetImmediatePredecessor(tls);
		break;

	case k_getImmediatePre:
		GetImmediatePredecessor(tls);
		break;

	case k_updFingerTable:
		UpdateFingerTable(tls);
		break;

	case k_dUpdFingerTable:
		DeUpdateFingerTable(tls);
		break;

	default:
		break;
	}
}

void Dht::ProcessStoreRequest(Decent::Net::TlsCommLayer & tls)
{
	using namespace EncFunc::Store;

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
}

void Dht::GetMigrateData(Decent::Net::TlsCommLayer & tls)
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

void Dht::SetMigrateData(Decent::Net::TlsCommLayer & tls)
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

void Dht::SetData(Decent::Net::TlsCommLayer & tls)
{
	std::array<uint8_t, DhtStates::sk_keySizeByte> keyBin{};
	tls.ReceiveRaw(keyBin.data(), keyBin.size());
	ConstBigNumber key(keyBin);

	//LOGI("Setting data for key %s.", key.Get().ToBigEndianHexStr().c_str());

	std::vector<uint8_t> buffer;
	tls.ReceiveMsg(buffer);

	gs_state.GetDhtStore().SetValue(key, std::move(buffer));

	tls.SendStruct(gsk_ack);
}

void Dht::GetData(Decent::Net::TlsCommLayer & tls)
{
	std::array<uint8_t, DhtStates::sk_keySizeByte> keyBin{};
	tls.ReceiveRaw(keyBin.data(), keyBin.size());
	ConstBigNumber key(keyBin);

	//LOGI("Getting data for key %s.", key.Get().ToBigEndianHexStr().c_str());

	std::vector<uint8_t> buffer;
	gs_state.GetDhtStore().GetValue(key, buffer);

	tls.SendMsg(buffer);
}

void Dht::DelData(Decent::Net::TlsCommLayer & tls)
{
	std::array<uint8_t, DhtStates::sk_keySizeByte> keyBin{};
	tls.ReceiveRaw(keyBin.data(), keyBin.size());
	ConstBigNumber key(keyBin);

	gs_state.GetDhtStore().DelValue(key);

	tls.SendStruct(gsk_ack);
}

void Dht::Init(uint64_t selfAddr, int isFirstNode, uint64_t exAddr)
{
	std::shared_ptr<DhtStates::DhtLocalNodeType::Pow2iArrayType> pow2iArray = std::make_shared<DhtStates::DhtLocalNodeType::Pow2iArrayType>();
	for (size_t i = 0; i < pow2iArray->size(); ++i)
	{
		(*pow2iArray)[i].SetBit(i, true);
	}

	BigNumber selfId = BigNumber::Rand(DhtStates::sk_keySizeByte); //For testing purpose, we use random ID.

	std::array<uint8_t, DhtStates::sk_keySizeByte> filledArray;
	memset_s(filledArray.data(), filledArray.size(), 0xFF, filledArray.size());
	MbedTlsObj::ConstBigNumber largest(filledArray);

	PRINT_I("Self Node ID: %s.", selfId.ToBigEndianHexStr().c_str());
	DhtStates::DhtLocalNodePtrType dhtNode = std::make_shared<DhtStates::DhtLocalNodeType>(selfId, selfAddr, 0, largest, pow2iArray);

	gs_state.GetDhtNode() = dhtNode;

	if (!isFirstNode)
	{
		NodeConnector nodeCnt(exAddr);
		dhtNode->Join(nodeCnt);

		uint64_t succAddr = dhtNode->GetImmediateSuccessor()->GetAddress();
		const BigNumber& predId = dhtNode->GetImmediatePredecessor()->GetNodeId();
		gs_state.GetDhtStore().MigrateFrom(succAddr, selfId, predId);
	}
}

void Dht::DeInit()
{
	DhtStates::DhtLocalNodePtrType dhtNode = gs_state.GetDhtNode();
	uint64_t succAddr = dhtNode->GetImmediateSuccessor()->GetAddress();
	dhtNode->Leave();
	if (dhtNode->GetAddress() != succAddr)
	{
		gs_state.GetDhtStore().MigrateTo(succAddr);
	}
}

void Dht::ProcessAppRequest(Decent::Net::TlsCommLayer & tls)
{
	using namespace EncFunc::App;

	NumType funcNum;
	tls.ReceiveStruct(funcNum); //1. Received function type.

	switch (funcNum)
	{
	case k_findSuccessor:
		FindSuccessor(tls);
		break;

	case k_getData:
		GetData(tls);
		break;

	case k_setData:
		SetData(tls);
		break;

	case k_delData:
		DelData(tls);
		break;

	default:
		break;
	}
}
