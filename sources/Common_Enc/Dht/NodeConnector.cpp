#include "NodeConnector.h"

#include <DecentApi/Common/Common.h>
#include <DecentApi/Common/make_unique.h>
#include <DecentApi/Common/Net/SecureCommLayer.h>

#include "../../Common/Dht/FuncNums.h"
#include "DhtStatesSingleton.h"
#include "DhtConnectionPool.h"

using namespace Decent;
using namespace Decent::Dht;
using namespace Decent::MbedTlsObj;
using namespace Decent::Ra;
using namespace Decent::Net;

namespace
{
	DhtStates& gs_state = GetDhtStatesSingleton();

	static char gsk_ack[] = "ACK";
}

void NodeConnector::SendNode(SecureCommLayer & comm, NodeBasePtr node)
{
	std::array<uint8_t, DhtStates::sk_keySizeByte> keyBin{};
	node->GetNodeId().ToBinary(keyBin);
	comm.SendRaw(keyBin.data(), keyBin.size()); //1. Sent resultant ID

	uint64_t addr = node->GetAddress();
	comm.SendStruct(addr); //2. Sent Address - Done!

	//LOGI("Sent result ID: %s.", node->GetNodeId().ToBigEndianHexStr().c_str());
}

void NodeConnector::SendNode(ConnectionBase& connection, SecureCommLayer & comm, NodeBasePtr node)
{
	comm.SetConnectionPtr(connection);
	SendNode(comm, node);
}

NodeConnector::NodeBasePtr NodeConnector::ReceiveNode(SecureCommLayer & comm)
{
	std::array<uint8_t, DhtStates::sk_keySizeByte> keyBin{};
	uint64_t addr;

	comm.ReceiveRaw(keyBin.data(), keyBin.size()); //1. Receive ID
	comm.ReceiveStruct(addr); //2. Receive Address.

	//LOGI("Recv result ID: %s.", BigNumber(keyBin).ToBigEndianHexStr().c_str());
	//LOGI("");
	return std::make_shared<NodeConnector>(addr, BigNumber(keyBin));
}

NodeConnector::NodeBasePtr NodeConnector::ReceiveNode(ConnectionBase& connection, SecureCommLayer & comm)
{
	comm.SetConnectionPtr(connection);
	return ReceiveNode(comm);
}

NodeConnector::NodeConnector(uint64_t address) :
	m_address(address),
	m_Id()
{}

NodeConnector::NodeConnector(uint64_t address, const MbedTlsObj::BigNumber & Id) :
	m_address(address),
	m_Id(Tools::make_unique<MbedTlsObj::BigNumber>(Id))
{
}

NodeConnector::NodeConnector(uint64_t address, MbedTlsObj::BigNumber && Id) :
	m_address(address),
	m_Id(Tools::make_unique<MbedTlsObj::BigNumber>(std::forward<MbedTlsObj::BigNumber>(Id)))
{
}

NodeConnector::~NodeConnector()
{
}

NodeConnector::NodeBasePtr NodeConnector::LookupTypeFunc(const MbedTlsObj::BigNumber & key, EncFunc::Dht::NumType type)
{
	CntPair cntPair = gs_state.GetConnectionPool().Get(m_address, gs_state);

	cntPair.GetCommLayer().SendStruct(type); //1. Send function type

	std::array<uint8_t, DhtStates::sk_keySizeByte> keyBin{};
	key.ToBinary(keyBin);
	cntPair.GetCommLayer().SendRaw(keyBin.data(), keyBin.size()); //2. Send queried ID
	//LOGI("Sent queried ID: %s.", key.ToBigEndianHexStr().c_str());

	NodeConnector::NodeBasePtr res = ReceiveNode(cntPair.GetCommLayer()); //3. Receive node. - Done!

	gs_state.GetConnectionPool().Put(m_address, std::move(cntPair));
	return res;
}

NodeConnector::NodeBasePtr NodeConnector::FindSuccessor(const BigNumber & key)
{
	//LOGI("Node Connector: Finding Successor of Node %s...", GetNodeId().ToBigEndianHexStr().c_str());
	using namespace EncFunc::Dht;
	return LookupTypeFunc(key, k_findSuccessor);
}

NodeConnector::NodeBasePtr NodeConnector::FindPredecessor(const MbedTlsObj::BigNumber & key)
{
	//LOGI("Node Connector: Finding Predecessor of Node %s...", GetNodeId().ToBigEndianHexStr().c_str());
	using namespace EncFunc::Dht;
	return LookupTypeFunc(key, k_findPredecessor);
}

NodeConnector::NodeBasePtr NodeConnector::GetImmediateSuccessor()
{
	//LOGI("Node Connector: Getting Immediate Successor of Node %s...", GetNodeId().ToBigEndianHexStr().c_str());
	using namespace EncFunc::Dht;

	CntPair cntPair = gs_state.GetConnectionPool().Get(m_address, gs_state);

	cntPair.GetCommLayer().SendStruct(k_getImmediateSuc); //1. Send function type

	NodeConnector::NodeBasePtr res = ReceiveNode(cntPair.GetCommLayer()); //2. Receive node. - Done!

	gs_state.GetConnectionPool().Put(m_address, std::move(cntPair));
	return res;
}

NodeConnector::NodeBasePtr NodeConnector::GetImmediatePredecessor()
{
	//LOGI("Node Connector: Getting Immediate Predecessor of Node %s...", GetNodeId().ToBigEndianHexStr().c_str());
	using namespace EncFunc::Dht;

	CntPair cntPair = gs_state.GetConnectionPool().Get(m_address, gs_state);

	cntPair.GetCommLayer().SendStruct(k_getImmediatePre); //1. Send function type

	NodeConnector::NodeBasePtr res = ReceiveNode(cntPair.GetCommLayer()); //2. Receive node. - Done!

	gs_state.GetConnectionPool().Put(m_address, std::move(cntPair));
	return res;

}

void NodeConnector::SetImmediatePredecessor(NodeBasePtr pred)
{
	//LOGI("Node Connector: Setting Immediate Predecessor of Node %s...", GetNodeId().ToBigEndianHexStr().c_str());
	using namespace EncFunc::Dht;

	CntPair cntPair = gs_state.GetConnectionPool().Get(m_address, gs_state);

	cntPair.GetCommLayer().SendStruct(k_setImmediatePre); //1. Send function type

	SendNode(cntPair.GetCommLayer(), pred); //2. Send Node. - Done!

	cntPair.GetCommLayer().ReceiveStruct(gsk_ack);
	//LOGI("");
	
	gs_state.GetConnectionPool().Put(m_address, std::move(cntPair));
}

void NodeConnector::UpdateFingerTable(NodeBasePtr & s, uint64_t i)
{
	//LOGI("Node Connector: Updating Finger Table of Node %s...", GetNodeId().ToBigEndianHexStr().c_str());
	using namespace EncFunc::Dht;

	CntPair cntPair = gs_state.GetConnectionPool().Get(m_address, gs_state);

	cntPair.GetCommLayer().SendStruct(k_updFingerTable); //1. Send function type
	SendNode(cntPair.GetCommLayer(), s); //2. Send Node.
	cntPair.GetCommLayer().SendStruct(i); //3. Send i. - Done!

	cntPair.GetCommLayer().ReceiveStruct(gsk_ack);
	//LOGI("");

	gs_state.GetConnectionPool().Put(m_address, std::move(cntPair));
}

void NodeConnector::DeUpdateFingerTable(const MbedTlsObj::BigNumber & oldId, NodeBasePtr & succ, uint64_t i)
{
	//LOGI("Node Connector: De-Updating Finger Table of Node %s...", GetNodeId().ToBigEndianHexStr().c_str());
	using namespace EncFunc::Dht;

	CntPair cntPair = gs_state.GetConnectionPool().Get(m_address, gs_state);

	cntPair.GetCommLayer().SendStruct(k_dUpdFingerTable); //1. Send function type

	std::array<uint8_t, DhtStates::sk_keySizeByte> keyBin{};
	oldId.ToBinary(keyBin);
	cntPair.GetCommLayer().SendRaw(keyBin.data(), keyBin.size()); //2. Send oldId.

	SendNode(cntPair.GetCommLayer(), succ); //3. Send Node.
	cntPair.GetCommLayer().SendStruct(i); //4. Send i. - Done!

	cntPair.GetCommLayer().ReceiveStruct(gsk_ack);
	//LOGI("");

	gs_state.GetConnectionPool().Put(m_address, std::move(cntPair));
}

const BigNumber & NodeConnector::GetNodeId()
{
	if (m_Id)
	{
		return *m_Id; //We already record the ID previously.
	}

	//If not, we need to query the peer:
	//LOGI("Node Connector: Getting Node ID...");
	using namespace EncFunc::Dht;

	CntPair cntPair = gs_state.GetConnectionPool().Get(m_address, gs_state);

	cntPair.GetCommLayer().SendStruct(k_getNodeId); //1. Send function type

	std::array<uint8_t, DhtStates::sk_keySizeByte> keyBin{};
	cntPair.GetCommLayer().ReceiveRaw(keyBin.data(), keyBin.size()); //2. Received resultant ID - Done!

	m_Id = Tools::make_unique<BigNumber>(keyBin);
	//LOGI("Recv result ID: %s.", m_Id->ToBigEndianHexStr().c_str());
	//LOGI("");

	gs_state.GetConnectionPool().Put(m_address, std::move(cntPair));
	return *m_Id;
}
