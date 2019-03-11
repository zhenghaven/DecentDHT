#include "../DhtStore.h"

#include <sgx_error.h>

#include <DecentApi/Common/Ra/TlsConfig.h>
#include <DecentApi/Common/Net/TlsCommLayer.h>
#include <DecentApi/CommonEnclave/SGX/OcallConnector.h>

#include "../DhtStatesSingleton.h"
#include "../../../Common/Dht/FuncNums.h"
#include "../../../Common/Dht/AppNames.h"

using namespace Decent;
using namespace Decent::Dht;

namespace
{
	DhtStates& gs_state = GetDhtStatesSingleton();
}

extern "C" sgx_status_t ocall_decent_dht_cnt_mgr_get_store(void** out_cnt_ptr, uint64_t address);

void DhtStore::MigrateFrom(const uint64_t & addr, const MbedTlsObj::BigNumber & start, const MbedTlsObj::BigNumber & end)
{
	using namespace EncFunc::Store;

	Net::OcallConnector connection(&ocall_decent_dht_cnt_mgr_get_store, addr);

	std::shared_ptr<Ra::TlsConfig> tlsCfg = std::make_shared<Ra::TlsConfig>(AppNames::sk_decentDHT, gs_state, false);
	Decent::Net::TlsCommLayer tls(connection.m_ptr, tlsCfg, true);

	tls.SendStruct(connection.m_ptr, k_getMigrateData); //1. Send function type

	std::array<uint8_t, DhtStates::sk_keySizeByte> keyBin{};

	start.ToBinary(keyBin);
	tls.SendRaw(connection.m_ptr, keyBin.data(), keyBin.size()); //2. Send start key.
	end.ToBinary(keyBin);
	tls.SendRaw(connection.m_ptr, keyBin.data(), keyBin.size()); //3. Send end key.

	RecvMigratingData([&connection, &tls](void* buffer, const size_t size) -> void
	{
		tls.ReceiveRaw(connection.m_ptr, buffer, size);
	}); //4. Receive data.
}

void DhtStore::MigrateTo(const uint64_t & addr)
{
	IndexingType sendIndexing;
	{
		std::unique_lock<std::mutex> indexingLock(GetIndexingMutex());
		sendIndexing.swap(GetIndexing());
	}

	using namespace EncFunc::Store;

	Net::OcallConnector connection(&ocall_decent_dht_cnt_mgr_get_store, addr);

	std::shared_ptr<Ra::TlsConfig> tlsCfg = std::make_shared<Ra::TlsConfig>(AppNames::sk_decentDHT, gs_state, false);
	Decent::Net::TlsCommLayer tls(connection.m_ptr, tlsCfg, true);

	tls.SendStruct(connection.m_ptr, k_setMigrateData); //1. Send function type

	SendMigratingData([&connection, &tls](void* buffer, const size_t size) -> void
	{
		tls.ReceiveRaw(connection.m_ptr, buffer, size);
	},
		sendIndexing); //2. Send data.
}
