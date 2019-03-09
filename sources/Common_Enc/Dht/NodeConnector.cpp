#include "NodeConnector.h"

#include <DecentApi/Common/Common.h>
#include <DecentApi/Common/make_unique.h>
#include <DecentApi/Common/Ra/TlsConfig.h>
#include <DecentApi/Common/Net/TlsCommLayer.h>

#include <sgx_error.h>

#include "../Connector.h"

#include "../../Common/Dht/AppNames.h"
#include "../../Common/Dht/FuncNums.h"
#include "DhtStatesSingleton.h"

extern "C" sgx_status_t ocall_decent_dht_cnt_mgr_get_dht(void** out_cnt_ptr, uint64_t address);

using namespace Decent;
using namespace Decent::Dht;
using namespace Decent::MbedTlsObj;
using namespace Decent::Ra;

namespace
{
	DhtStates& gs_state = Dht::GetDhtStatesSingleton();
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

NodeConnector::NodeBaseType * NodeConnector::LookupTypeFunc(const MbedTlsObj::BigNumber & key, EncFunc::Dht::NumType type)
{
	EnclaveConnector connection(&ocall_decent_dht_cnt_mgr_get_dht, m_address);

	std::shared_ptr<Ra::TlsConfig> tlsCfg = std::make_shared<Ra::TlsConfig>(AppNames::sk_decentDHT, gs_state, false);
	Decent::Net::TlsCommLayer tls(connection.m_ptr, tlsCfg, true);

	tls.SendStruct(connection.m_ptr, type); //1. Send function type

	std::array<uint8_t, DhtStates::sk_keySizeByte> keyBin{};
	key.ToBinary(keyBin);
	tls.SendRaw(connection.m_ptr, keyBin.data(), keyBin.size()); //2. Send queried ID
	LOGI("Sent queried ID: %s.", key.ToBigEndianHexStr().c_str());

	tls.ReceiveRaw(connection.m_ptr, keyBin.data(), keyBin.size()); //3. Received resultant ID

	uint64_t resAddr = 0;
	tls.ReceiveStruct(connection.m_ptr, resAddr); //4. Received Address - Done!

	BigNumber resId(keyBin);
	LOGI("Recv result ID: %s.", resId.ToBigEndianHexStr().c_str());

	//return std::make_shared<NodeConnector>(resAddr, std::move(resId));
	return nullptr;
}

NodeConnector::NodeBaseType* NodeConnector::FindSuccessor(const BigNumber & key)
{
	LOGI("Finding Successor...");
	using namespace EncFunc::Dht;
	return LookupTypeFunc(key, k_findSuccessor);
}

NodeConnector::NodeBaseType * Decent::Dht::NodeConnector::FindPredecessor(const MbedTlsObj::BigNumber & key)
{
	LOGI("Finding Predecessor...");
	using namespace EncFunc::Dht;
	return LookupTypeFunc(key, k_findPredecessor);
}

NodeConnector::NodeBaseType * Decent::Dht::NodeConnector::GetImmediateSuccessor()
{
	LOGI("Getting Immediate Successor...");
	using namespace EncFunc::Dht;

	EnclaveConnector connection(&ocall_decent_dht_cnt_mgr_get_dht, m_address);

	std::shared_ptr<Ra::TlsConfig> tlsCfg = std::make_shared<Ra::TlsConfig>(AppNames::sk_decentDHT, gs_state, false);
	Decent::Net::TlsCommLayer tls(connection.m_ptr, tlsCfg, true);

	tls.SendStruct(connection.m_ptr, k_getImmediateSuc); //1. Send function type

	std::array<uint8_t, DhtStates::sk_keySizeByte> keyBin{};
	tls.ReceiveRaw(connection.m_ptr, keyBin.data(), keyBin.size()); //2. Received resultant ID

	uint64_t resAddr = 0;
	tls.ReceiveStruct(connection.m_ptr, resAddr); //3. Received Address - Done!

	BigNumber resId(keyBin);
	LOGI("Recv result ID: %s.", resId.ToBigEndianHexStr().c_str());

	//return std::make_shared<NodeConnector>(resAddr, std::move(resId));
	return nullptr;
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

	EnclaveConnector connection(&ocall_decent_dht_cnt_mgr_get_dht, m_address);

	std::shared_ptr<Ra::TlsConfig> tlsCfg = std::make_shared<Ra::TlsConfig>(AppNames::sk_decentDHT, gs_state, false);
	Decent::Net::TlsCommLayer tls(connection.m_ptr, tlsCfg, true);

	tls.SendStruct(connection.m_ptr, k_getNodeId); //1. Send function type

	std::array<uint8_t, DhtStates::sk_keySizeByte> keyBin{};
	tls.ReceiveRaw(connection.m_ptr, keyBin.data(), keyBin.size()); //2. Received resultant ID - Done!

	m_Id = Tools::make_unique<BigNumber>(keyBin);
	LOGI("Recv result ID: %s.", m_Id->ToBigEndianHexStr().c_str());

	return *m_Id;
}
