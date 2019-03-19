#include "NodeConnector.h"

#include <DecentApi/Common/Common.h>
#include <DecentApi/Common/make_unique.h>
#include <DecentApi/Common/Net/TlsCommLayer.h>
#include <DecentApi/CommonEnclave/Ra/TlsConfigSameEnclave.h>
#include <DecentApi/CommonEnclave/Net/EnclaveNetConnector.h>

#include "../../Common/Dht/FuncNums.h"
#include "DhtStatesSingleton.h"
#include "ConnectionManager.h"

using namespace Decent;
using namespace Decent::Dht;
using namespace Decent::MbedTlsObj;
using namespace Decent::Ra;
using namespace Decent::Net;

namespace
{
	DhtStates& gs_state = GetDhtStatesSingleton();

	static char gsk_ack[] = "ACK";

	static std::shared_ptr<Ra::TlsConfigSameEnclave> GetClientTlsConfigDhtNode()
	{
		static std::shared_ptr<Ra::TlsConfigSameEnclave> tlsCfg = std::make_shared<Ra::TlsConfigSameEnclave>(gs_state, Ra::TlsConfig::Mode::ClientHasCert);
		return tlsCfg;
	}
}

void NodeConnector::SendNode(TlsCommLayer & tls, NodeBasePtr node)
{
	std::array<uint8_t, DhtStates::sk_keySizeByte> keyBin{};
	node->GetNodeId().ToBinary(keyBin);
	tls.SendRaw(keyBin.data(), keyBin.size()); //1. Sent resultant ID

	uint64_t addr = node->GetAddress();
	tls.SendStruct(addr); //2. Sent Address - Done!

	//LOGI("Sent result ID: %s.", node->GetNodeId().ToBigEndianHexStr().c_str());
}

void NodeConnector::SendNode(void * connection, TlsCommLayer & tls, NodeBasePtr node)
{
	tls.SetConnectionPtr(connection);
	SendNode(tls, node);
}

NodeConnector::NodeBasePtr Decent::Dht::NodeConnector::ReceiveNode(TlsCommLayer & tls)
{
	std::array<uint8_t, DhtStates::sk_keySizeByte> keyBin{};
	uint64_t addr;

	tls.ReceiveRaw(keyBin.data(), keyBin.size()); //1. Receive ID
	tls.ReceiveStruct(addr); //2. Receive Address.

	//LOGI("Recv result ID: %s.", BigNumber(keyBin).ToBigEndianHexStr().c_str());
	//LOGI("");
	return std::make_shared<NodeConnector>(addr, BigNumber(keyBin));
}

NodeConnector::NodeBasePtr NodeConnector::ReceiveNode(void * connection, TlsCommLayer & tls)
{
	tls.SetConnectionPtr(connection);
	return ReceiveNode(tls);
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
	std::unique_ptr<EnclaveNetConnector> connection = ConnectionManager::GetConnection2DecentNode(m_address);
	Decent::Net::TlsCommLayer tls(connection->Get(), GetClientTlsConfigDhtNode(), true);

	tls.SendStruct(type); //1. Send function type

	std::array<uint8_t, DhtStates::sk_keySizeByte> keyBin{};
	key.ToBinary(keyBin);
	tls.SendRaw(keyBin.data(), keyBin.size()); //2. Send queried ID
	//LOGI("Sent queried ID: %s.", key.ToBigEndianHexStr().c_str());

	return ReceiveNode(tls); //3. Receive node. - Done!
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

	std::unique_ptr<EnclaveNetConnector> connection = ConnectionManager::GetConnection2DecentNode(m_address);
	Decent::Net::TlsCommLayer tls(connection->Get(), GetClientTlsConfigDhtNode(), true);

	tls.SendStruct(k_getImmediateSuc); //1. Send function type

	return ReceiveNode(tls); //2. Receive node. - Done!
}

NodeConnector::NodeBasePtr NodeConnector::GetImmediatePredecessor()
{
	//LOGI("Node Connector: Getting Immediate Predecessor of Node %s...", GetNodeId().ToBigEndianHexStr().c_str());
	using namespace EncFunc::Dht;

	std::unique_ptr<EnclaveNetConnector> connection = ConnectionManager::GetConnection2DecentNode(m_address);
	Decent::Net::TlsCommLayer tls(connection->Get(), GetClientTlsConfigDhtNode(), true);

	tls.SendStruct(k_getImmediatePre); //1. Send function type

	return ReceiveNode(tls); //2. Receive node. - Done!

}

void NodeConnector::SetImmediatePredecessor(NodeBasePtr pred)
{
	//LOGI("Node Connector: Setting Immediate Predecessor of Node %s...", GetNodeId().ToBigEndianHexStr().c_str());
	using namespace EncFunc::Dht;

	std::unique_ptr<EnclaveNetConnector> connection = ConnectionManager::GetConnection2DecentNode(m_address);
	Decent::Net::TlsCommLayer tls(connection->Get(), GetClientTlsConfigDhtNode(), true);

	tls.SendStruct(k_setImmediatePre); //1. Send function type

	SendNode(tls, pred); //2. Send Node. - Done!

	tls.ReceiveStruct(gsk_ack);
	//LOGI("");
}

void NodeConnector::UpdateFingerTable(NodeBasePtr & s, uint64_t i)
{
	//LOGI("Node Connector: Updating Finger Table of Node %s...", GetNodeId().ToBigEndianHexStr().c_str());
	using namespace EncFunc::Dht;

	std::unique_ptr<EnclaveNetConnector> connection = ConnectionManager::GetConnection2DecentNode(m_address);
	Decent::Net::TlsCommLayer tls(connection->Get(), GetClientTlsConfigDhtNode(), true);

	tls.SendStruct(k_updFingerTable); //1. Send function type
	SendNode(tls, s); //2. Send Node.
	tls.SendStruct(i); //3. Send i. - Done!

	tls.ReceiveStruct(gsk_ack);
	//LOGI("");
}

void NodeConnector::DeUpdateFingerTable(const MbedTlsObj::BigNumber & oldId, NodeBasePtr & succ, uint64_t i)
{
	//LOGI("Node Connector: De-Updating Finger Table of Node %s...", GetNodeId().ToBigEndianHexStr().c_str());
	using namespace EncFunc::Dht;

	std::unique_ptr<EnclaveNetConnector> connection = ConnectionManager::GetConnection2DecentNode(m_address);
	Decent::Net::TlsCommLayer tls(connection->Get(), GetClientTlsConfigDhtNode(), true);

	tls.SendStruct(k_dUpdFingerTable); //1. Send function type

	std::array<uint8_t, DhtStates::sk_keySizeByte> keyBin{};
	oldId.ToBinary(keyBin);
	tls.SendRaw(keyBin.data(), keyBin.size()); //2. Send oldId.

	SendNode(tls, succ); //3. Send Node.
	tls.SendStruct(i); //4. Send i. - Done!

	tls.ReceiveStruct(gsk_ack);
	//LOGI("");
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

	std::unique_ptr<EnclaveNetConnector> connection = ConnectionManager::GetConnection2DecentNode(m_address);
	Decent::Net::TlsCommLayer tls(connection->Get(), GetClientTlsConfigDhtNode(), true);

	tls.SendStruct(k_getNodeId); //1. Send function type

	std::array<uint8_t, DhtStates::sk_keySizeByte> keyBin{};
	tls.ReceiveRaw(keyBin.data(), keyBin.size()); //2. Received resultant ID - Done!

	m_Id = Tools::make_unique<BigNumber>(keyBin);
	//LOGI("Recv result ID: %s.", m_Id->ToBigEndianHexStr().c_str());
	//LOGI("");

	return *m_Id;
}
