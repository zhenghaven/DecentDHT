#include "EnclaveStore.h"

#include <DecentApi/Common/Common.h>
#include <DecentApi/Common/Net/TlsCommLayer.h>
#include <DecentApi/Common/Tools/FileSystemUtil.h>
#include <DecentApi/CommonEnclave/Ra/TlsConfigSameEnclave.h>
#include <DecentApi/CommonEnclave/Net/EnclaveNetConnector.h>
#include <DecentApi/CommonEnclave/Tools/Crypto.h>
#include <DecentApi/CommonEnclave/Tools/PlainFile.h>
#include <DecentApi/CommonEnclave/Tools/SecureFile.h>

#include "../../Common/Dht/LocalNode.h"
#include "../../Common/Dht/FuncNums.h"

#include "ConnectionManager.h"
#include "DhtStatesSingleton.h"

using namespace Decent;
using namespace Decent::Dht;
using namespace Decent::Net;
using namespace Decent::MbedTlsObj;

namespace
{
	DhtStates& gs_state = GetDhtStatesSingleton();

	static std::shared_ptr<Ra::TlsConfigSameEnclave> GetClientTlsConfigDhtNode()
	{
		static std::shared_ptr<Ra::TlsConfigSameEnclave> tlsCfg = std::make_shared<Ra::TlsConfigSameEnclave>(gs_state, Ra::TlsConfig::Mode::ClientHasCert);
		return tlsCfg;
	}

	constexpr char const gsk_storeSealKeyLabel[] = "DecentStoreSealKey";
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
	using namespace Decent::Tools;

	const std::string fileName = key.ToBigEndianHexStr();
	
	std::vector<uint8_t> meta;
	try
	{
		PlainFile metaFile(fileName + ".meta", FileBase::Mode::Read);
		meta.resize(metaFile.GetFileSize());
		metaFile.ReadBlockExactSize(meta);
	}
	catch (const Decent::RuntimeException&)
	{
		data.resize(0);
		return;
	}

	General128BitKey sealKey;
	DeriveSealKey(KeyPolicy::ByMrEnclave, gsk_storeSealKeyLabel, sealKey, meta);

	try
	{
		SecureFile dataFile(fileName + ".data", sealKey, FileBase::Mode::Read);
		data.resize(dataFile.GetFileSize());
		dataFile.ReadBlockExactSize(data);
	}
	catch (const Decent::RuntimeException&)
	{
		data.resize(0);
		return;
	}
}

void EnclaveStore::DeleteDataFile(const MbedTlsObj::BigNumber & key)
{
	const std::string fileName = key.ToBigEndianHexStr();
	Tools::FileSysDeleteFile(fileName + ".meta");
	Tools::FileSysDeleteFile(fileName + ".data");
}

void EnclaveStore::SaveData(const MbedTlsObj::BigNumber & key, std::vector<uint8_t>&& data)
{
	EnclaveStore::SaveData(BigNumber(key), std::forward<std::vector<uint8_t> >(data));
}

void EnclaveStore::SaveData(MbedTlsObj::BigNumber&& key, std::vector<uint8_t>&& data)
{
	using namespace Decent::Tools;

	std::string fileName = key.ToBigEndianHexStr();
	LOGI("DHT store: adding key to the index. %s", fileName.c_str());

	StoreBase::SaveData(std::forward<MbedTlsObj::BigNumber>(key), std::forward<std::vector<uint8_t> >(data));
	//NOTE: *Key* is not valid after this line!!!

	LOGI("DHT store: writing value: %s", std::string(reinterpret_cast<const char*>(data.data()), data.size()).c_str());
	
	std::vector<uint8_t> meta;
	GenSealKeyRecoverMeta(meta);
	
	General128BitKey sealKey;
	DeriveSealKey(KeyPolicy::ByMrEnclave, gsk_storeSealKeyLabel, sealKey, meta);
	
	{
		WritablePlainFile metaFile(fileName + ".meta", WritableFileBase::WritableMode::Write);
		metaFile.WriteBlockExactSize(meta);
	}
	
	{
		WritableSecureFile dataFile(fileName + ".data", sealKey, WritableFileBase::WritableMode::Write);
		dataFile.WriteBlockExactSize(data);
	}
}
