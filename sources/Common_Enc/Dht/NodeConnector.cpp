#include "NodeConnector.h"

#include <DecentApi/Common/Common.h>
#include <DecentApi/Common/make_unique.h>
#include <DecentApi/Common/Ra/TlsConfig.h>
#include <DecentApi/Common/Net/TlsCommLayer.h>
#include <DecentApi/CommonEnclave/SGX/OcallConnector.h>

#include <sgx_error.h>

#include "../../Common/Dht/AppNames.h"
#include "../../Common/Dht/FuncNums.h"
#include "DhtStatesSingleton.h"

extern "C" sgx_status_t ocall_decent_dht_cnt_mgr_get_dht(void** out_cnt_ptr, uint64_t address);

using namespace Decent;
using namespace Decent::Dht;
using namespace Decent::MbedTlsObj;
using namespace Decent::Ra;
using namespace Decent::Net;

namespace
{
	DhtStates& gs_state = GetDhtStatesSingleton();
}

void NodeConnector::SendNode(void * connection, TlsCommLayer & tls, NodeBasePtrType node)
{
	std::array<uint8_t, DhtStates::sk_keySizeByte> keyBin{};
	node->GetNodeId().ToBinary(keyBin);
	tls.SendRaw(connection, keyBin.data(), keyBin.size()); //1. Sent resultant ID

	uint64_t addr = node->GetAddress();
	tls.SendStruct(connection, addr); //2. Sent Address - Done!

	LOGI("Sent result ID: %s.", node->GetNodeId().ToBigEndianHexStr().c_str());
}

NodeConnector::NodeBasePtrType NodeConnector::ReceiveNode(void * connection, TlsCommLayer & tls)
{
	std::array<uint8_t, DhtStates::sk_keySizeByte> keyBin{};
	uint64_t addr;

	tls.ReceiveRaw(connection, keyBin.data(), keyBin.size()); //1. Receive ID
	tls.ReceiveStruct(connection, addr); //2. Receive Address.

	LOGI("Recv result ID: %s.", BigNumber(keyBin).ToBigEndianHexStr().c_str());

	return std::make_shared<NodeConnector>(addr, BigNumber(keyBin));
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

NodeConnector::NodeBasePtrType NodeConnector::LookupTypeFunc(const MbedTlsObj::BigNumber & key, EncFunc::Dht::NumType type)
{
	Net::OcallConnector connection(&ocall_decent_dht_cnt_mgr_get_dht, m_address);

	std::shared_ptr<Ra::TlsConfig> tlsCfg = std::make_shared<Ra::TlsConfig>(AppNames::sk_decentDHT, gs_state, false);
	Decent::Net::TlsCommLayer tls(connection.m_ptr, tlsCfg, true);

	tls.SendStruct(connection.m_ptr, type); //1. Send function type

	std::array<uint8_t, DhtStates::sk_keySizeByte> keyBin{};
	key.ToBinary(keyBin);
	tls.SendRaw(connection.m_ptr, keyBin.data(), keyBin.size()); //2. Send queried ID
	LOGI("Sent queried ID: %s.", key.ToBigEndianHexStr().c_str());

	return ReceiveNode(connection.m_ptr, tls); //3. Receive node. - Done!
}

NodeConnector::NodeBasePtrType NodeConnector::FindSuccessor(const BigNumber & key)
{
	LOGI("Finding Successor...");
	using namespace EncFunc::Dht;
	return LookupTypeFunc(key, k_findSuccessor);
}

NodeConnector::NodeBasePtrType NodeConnector::FindPredecessor(const MbedTlsObj::BigNumber & key)
{
	LOGI("Finding Predecessor...");
	using namespace EncFunc::Dht;
	return LookupTypeFunc(key, k_findPredecessor);
}

NodeConnector::NodeBasePtrType NodeConnector::GetImmediateSuccessor()
{
	LOGI("Getting Immediate Successor...");
	using namespace EncFunc::Dht;

	Net::OcallConnector connection(&ocall_decent_dht_cnt_mgr_get_dht, m_address);

	std::shared_ptr<Ra::TlsConfig> tlsCfg = std::make_shared<Ra::TlsConfig>(AppNames::sk_decentDHT, gs_state, false);
	Decent::Net::TlsCommLayer tls(connection.m_ptr, tlsCfg, true);

	tls.SendStruct(connection.m_ptr, k_getImmediateSuc); //1. Send function type

	return ReceiveNode(connection.m_ptr, tls); //2. Receive node. - Done!
}

NodeConnector::NodeBasePtrType NodeConnector::GetImmediatePredecessor()
{

	LOGI("Getting Immediate Predecessor...");
	using namespace EncFunc::Dht;

	Net::OcallConnector connection(&ocall_decent_dht_cnt_mgr_get_dht, m_address);

	std::shared_ptr<Ra::TlsConfig> tlsCfg = std::make_shared<Ra::TlsConfig>(AppNames::sk_decentDHT, gs_state, false);
	Decent::Net::TlsCommLayer tls(connection.m_ptr, tlsCfg, true);

	tls.SendStruct(connection.m_ptr, k_getImmediatePre); //1. Send function type

	return ReceiveNode(connection.m_ptr, tls); //2. Receive node. - Done!

}

void NodeConnector::SetImmediatePredecessor(NodeBasePtrType pred)
{
	using namespace EncFunc::Dht;
	Net::OcallConnector connection(&ocall_decent_dht_cnt_mgr_get_dht, m_address);

	std::shared_ptr<Ra::TlsConfig> tlsCfg = std::make_shared<Ra::TlsConfig>(AppNames::sk_decentDHT, gs_state, false);
	Decent::Net::TlsCommLayer tls(connection.m_ptr, tlsCfg, true);

	tls.SendStruct(connection.m_ptr, k_setImmediatePre); //1. Send function type

	SendNode(connection.m_ptr, tls, pred); //2. Send Node. - Done!
}

void NodeConnector::UpdateFingerTable(NodeBasePtrType & s, uint64_t i)
{
	using namespace EncFunc::Dht;
	Net::OcallConnector connection(&ocall_decent_dht_cnt_mgr_get_dht, m_address);

	std::shared_ptr<Ra::TlsConfig> tlsCfg = std::make_shared<Ra::TlsConfig>(AppNames::sk_decentDHT, gs_state, false);
	Decent::Net::TlsCommLayer tls(connection.m_ptr, tlsCfg, true);

	tls.SendStruct(connection.m_ptr, k_updFingerTable); //1. Send function type

	SendNode(connection.m_ptr, tls, s); //2. Send Node.
	tls.SendStruct(connection.m_ptr,i); //3. Send i. - Done!
}

void NodeConnector::DeUpdateFingerTable(const MbedTlsObj::BigNumber & oldId, NodeBasePtrType & succ, uint64_t i)
{

	using namespace EncFunc::Dht;
	Net::OcallConnector connection(&ocall_decent_dht_cnt_mgr_get_dht, m_address);
	std::shared_ptr<Ra::TlsConfig> tlsCfg = std::make_shared<Ra::TlsConfig>(AppNames::sk_decentDHT, gs_state, false);
	Decent::Net::TlsCommLayer tls(connection.m_ptr, tlsCfg, true);

	tls.SendStruct(connection.m_ptr, k_dUpdFingerTable); //1. Send function type

	std::array<uint8_t, DhtStates::sk_keySizeByte> keyBin{};
	oldId.ToBinary(keyBin);
	tls.SendRaw(connection.m_ptr, keyBin.data(), keyBin.size()); //2. Send oldId.

	SendNode(connection.m_ptr, tls, succ); //3. Send Node.
	tls.SendStruct(connection.m_ptr,i); //4. Send i. - Done!
}

const BigNumber & NodeConnector::GetNodeId()
{
	if (m_Id)
	{
		return *m_Id; //We already record the ID previously.
	}

	//If not, we need to query the peer:
	LOGI("Getting Node ID...");
	using namespace EncFunc::Dht;

	Net::OcallConnector connection(&ocall_decent_dht_cnt_mgr_get_dht, m_address);

	std::shared_ptr<Ra::TlsConfig> tlsCfg = std::make_shared<Ra::TlsConfig>(AppNames::sk_decentDHT, gs_state, false);
	Decent::Net::TlsCommLayer tls(connection.m_ptr, tlsCfg, true);

	tls.SendStruct(connection.m_ptr, k_getNodeId); //1. Send function type

	std::array<uint8_t, DhtStates::sk_keySizeByte> keyBin{};
	tls.ReceiveRaw(connection.m_ptr, keyBin.data(), keyBin.size()); //2. Received resultant ID - Done!

	m_Id = Tools::make_unique<BigNumber>(keyBin);
	LOGI("Recv result ID: %s.", m_Id->ToBigEndianHexStr().c_str());

	return *m_Id;
}
