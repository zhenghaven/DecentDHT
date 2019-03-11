#include "EnclaveStore.h"

#include <DecentApi/Common/Common.h>
#include <DecentApi/Common/Ra/TlsConfig.h>
#include <DecentApi/Common/Net/TlsCommLayer.h>
#include <DecentApi/CommonEnclave/Net/EnclaveNetConnector.h>

#include "../../Common/Dht/LocalNode.h"
#include "../../Common/Dht/FuncNums.h"
#include "../../Common/Dht/AppNames.h"

#include "ConnectionManager.h"
#include "DhtStatesSingleton.h"

using namespace Decent;
using namespace Decent::Dht;
using namespace Decent::Net;
using namespace Decent::MbedTlsObj;

namespace
{
	DhtStates& gs_state = GetDhtStatesSingleton();

	static std::shared_ptr<Ra::TlsConfig> GetClientTlsConfigDhtNode()
	{
		static std::shared_ptr<Ra::TlsConfig> tlsCfg = std::make_shared<Ra::TlsConfig>(AppNames::sk_decentDHT, gs_state, false);
		return tlsCfg;
	}
}

EnclaveStore::~EnclaveStore()
{
}

void EnclaveStore::MigrateFrom(const uint64_t & addr, const MbedTlsObj::BigNumber & start, const MbedTlsObj::BigNumber & end)
{
	LOGI("Migrating data from peer...");
	using namespace EncFunc::Store;

	std::unique_ptr<EnclaveNetConnector> connection = ConnectionManager::GetConnection2DecentStore(addr);
	Decent::Net::TlsCommLayer tls(connection->Get(), GetClientTlsConfigDhtNode(), true);

	tls.SendStruct(k_getMigrateData); //1. Send function type

	std::array<uint8_t, DhtStates::sk_keySizeByte> keyBin{};

	start.ToBinary(keyBin);
	tls.SendRaw(keyBin.data(), keyBin.size()); //2. Send start key.
	end.ToBinary(keyBin);
	tls.SendRaw(keyBin.data(), keyBin.size()); //3. Send end key.

	RecvMigratingData(
		[&tls](void* buffer, const size_t size) -> void
	{
		tls.ReceiveRaw(buffer, size);
	}, 
	[&tls]() -> BigNumber
	{
		std::array<uint8_t, DhtStates::sk_keySizeByte> keyBuf{};
		tls.ReceiveRaw(keyBuf.data(), keyBuf.size());
		return BigNumber(keyBuf);
	}); //4. Receive data.
}

void EnclaveStore::MigrateTo(const uint64_t & addr)
{
	LOGI("Migrating data to peer...");
	using namespace EncFunc::Store;

	IndexingType sendIndexing;
	{
		std::unique_lock<std::mutex> indexingLock(GetIndexingMutex());
		sendIndexing.swap(GetIndexing());
	}


	std::unique_ptr<EnclaveNetConnector> connection = ConnectionManager::GetConnection2DecentStore(addr);
	Decent::Net::TlsCommLayer tls(connection->Get(), GetClientTlsConfigDhtNode(), true);

	tls.SendStruct(k_setMigrateData); //1. Send function type

	SendMigratingData(
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
		sendIndexing); //2. Send data.
}

bool EnclaveStore::IsResponsibleFor(const MbedTlsObj::BigNumber & key) const
{
	DhtStates::DhtLocalNodePtrType localNode = gs_state.GetDhtNode();
	return localNode ? localNode->IsResponsibleFor(key) : false;
}

void EnclaveStore::GetValue(const MbedTlsObj::BigNumber & key, std::vector<uint8_t>& data)
{
	//TODO: work with file system.
}

std::vector<uint8_t> EnclaveStore::DeleteData(const MbedTlsObj::BigNumber & key)
{
	//TODO: work with file system.
	return std::vector<uint8_t>();
}

void EnclaveStore::SaveData(const MbedTlsObj::BigNumber & key, std::vector<uint8_t>&& data)
{
	EnclaveStore::SaveData(BigNumber(key), std::forward<std::vector<uint8_t> >(data));
}

void EnclaveStore::SaveData(MbedTlsObj::BigNumber&& key, std::vector<uint8_t>&& data)
{
	StoreBase::SaveData(std::forward<MbedTlsObj::BigNumber >(key), std::forward<std::vector<uint8_t> >(data));
	//TODO: work with file system.
}
