#include "DhtServer.h"

#include <DecentApi/Common/Common.h>
#include <DecentApi/Common/Ra/TlsConfig.h>
#include <DecentApi/Common/Net/TlsCommLayer.h>
#include <DecentApi/Common/MbedTls/BigNumber.h>

#include "../../Common/Dht/LocalNode.h"
#include "../../Common/Dht/FuncNums.h"
#include "../../Common/Dht/AppNames.h"
#include "../../Common/Dht/NodeBase.h"

#include "DhtStatesSingleton.h"
#include "NodeConnector.h"

using namespace Decent;
using namespace Decent::Dht;
using namespace Decent::MbedTlsObj;

namespace
{
	DhtStates& gs_state = Dht::GetDhtStatesSingleton();
}

void Dht::DeUpdateFingerTable(void* connection, Decent::Net::TlsCommLayer &tls)
{
	std::array<uint8_t, DhtStates::sk_keySizeByte> oldIdBin{};
	uint64_t i;

	tls.ReceiveRaw(connection, oldIdBin.data(), oldIdBin.size()); //2. Receive old ID.

	DhtStates::DhtLocalNodeType::NodeBasePtr s = NodeConnector::ReceiveNode(connection, tls); //3. Receive node.
	tls.ReceiveStruct(connection, i); //3. Receive i. - Done!

	DhtStates::DhtLocalNodePtrType localNode = gs_state.GetDhtNode();

	localNode->DeUpdateFingerTable(ConstBigNumber(oldIdBin), s, i);
}

void Dht::UpdateFingerTable(void* connection, Decent::Net::TlsCommLayer &tls)
{
	DhtStates::DhtLocalNodeType::NodeBasePtr s = NodeConnector::ReceiveNode(connection, tls); //2. Receive node.

	uint64_t i;
	tls.ReceiveStruct(connection, i); //3. Receive i. - Done!

	DhtStates::DhtLocalNodePtrType localNode = gs_state.GetDhtNode();

	localNode->UpdateFingerTable(s, i);
}

void Dht::GetImmediatePredecessor(void* connection, Decent::Net::TlsCommLayer &tls)
{
	LOGI("Getting Immediate Predecessor...");

	DhtStates::DhtLocalNodePtrType localNode = gs_state.GetDhtNode();

	NodeConnector::SendNode(connection, tls, localNode->GetImmediatePredecessor()); //2. Send Node. - Done!
}

void Dht::SetImmediatePredecessor(void* connection, Decent::Net::TlsCommLayer &tls)
{
	LOGI("Setting Immediate Predecessor...");

	DhtStates::DhtLocalNodePtrType localNode = gs_state.GetDhtNode();

	localNode->SetImmediatePredecessor(NodeConnector::ReceiveNode(connection, tls)); //2. Receive Node. - Done!
}

void Dht::GetImmediateSucessor(void* connection, Decent::Net::TlsCommLayer &tls)
{
    LOGI("Finding Immediate Successor...");

    DhtStates::DhtLocalNodePtrType localNode = gs_state.GetDhtNode();

	NodeConnector::SendNode(connection, tls, localNode->GetImmediateSuccessor()); //2. Send Node. - Done!
}

void Dht::GetNodeId(void* connection, Decent::Net::TlsCommLayer &tls)
{
    LOGI("Getting the NodeId...");
	DhtStates::DhtLocalNodePtrType localNode = gs_state.GetDhtNode();
    std::array<uint8_t, DhtStates::sk_keySizeByte> keyBin{};
    localNode->GetNodeId().ToBinary(keyBin);

    tls.SendRaw(connection, keyBin.data(), keyBin.size()); //2. Sent nodeId ID

    LOGI("Sent result ID: %s.", localNode->GetNodeId().ToBigEndianHexStr().c_str());
}

void Dht::FindPredecessor(void* connection, Decent::Net::TlsCommLayer &tls)
{
    LOGI("Finding Predecessor...");
    std::array<uint8_t, DhtStates::sk_keySizeByte> keyBin{};
    tls.ReceiveRaw(connection, keyBin.data(), keyBin.size()); //2. Received queried ID

    ConstBigNumber queriedId(keyBin);
    LOGI("Recv queried ID: %s.", static_cast<const BigNumber&>(queriedId).ToBigEndianHexStr().c_str());

	DhtStates::DhtLocalNodePtrType localNode = gs_state.GetDhtNode();

	NodeConnector::SendNode(connection, tls, localNode->FindPredecessor(queriedId)); //3. Send Node. - Done!
}

void Dht::FindSuccessor(void* connection, Decent::Net::TlsCommLayer &tls)
{
	LOGI("Finding Successor...");
	std::array<uint8_t, DhtStates::sk_keySizeByte> keyBin{};
	tls.ReceiveRaw(connection, keyBin.data(), keyBin.size()); //2. Received queried ID

	ConstBigNumber queriedId(keyBin);
	LOGI("Recv queried ID: %s.", static_cast<const BigNumber&>(queriedId).ToBigEndianHexStr().c_str());

	DhtStates::DhtLocalNodePtrType localNode = gs_state.GetDhtNode();

	NodeConnector::SendNode(connection, tls, localNode->FindSuccessor(queriedId)); //3. Send Node. - Done!
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
        FindPredecessor(connection, tls);
		break;

	case k_getImmediateSuc:
        GetImmediateSucessor(connection,tls);
		break;

	case k_getNodeId:
	    GetNodeId(connection, tls);
		break;

	case k_setImmediatePre:
		SetImmediatePredecessor(connection, tls);
		break;

	case k_getImmediatePre:
		GetImmediatePredecessor(connection, tls);
		break;

	case k_updFingerTable:
		UpdateFingerTable(connection, tls);
		break;

	case k_dUpdFingerTable:
		DeUpdateFingerTable(connection, tls);
		break;

	default:
		break;
	}
}
