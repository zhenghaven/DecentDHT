//#if ENCLAVE_PLATFORM_SGX

#include "../EnclaveStore.h"

#include <DecentApi/Common/Common.h>
#include <DecentApi/Common/SGX/ErrorCode.h>
#include <DecentApi/CommonEnclave/Tools/UntrustedBuffer.h>
#include <DecentApi/CommonEnclave/Tools/DataSealer.h>

#include "../../../Common/Dht/LocalNode.h"

#include "../DhtStatesSingleton.h"

#include "edl_decent_dht_mem_store.h"

using namespace Decent;
using namespace Decent::Dht;
using namespace Decent::Tools;

namespace
{
	DhtStates& gs_state = GetDhtStatesSingleton();

	const std::vector<uint8_t>& GetSealKeyMetaData()
	{
		static std::vector<uint8_t> inst = DataSealer::GenSealKeyRecoverMeta();
		return inst;
	}

	void* InitializeMemStore()
	{
		void* res = nullptr;
		sgx_status_t sgxRet = ocall_decent_dht_mem_store_init(&res);

		if (sgxRet != SGX_SUCCESS)
		{
			throw RuntimeException(Sgx::ConstructSimpleErrorMsg(sgxRet, "ocall_decent_dht_mem_store_init"));
		}
		if (res == nullptr)
		{
			throw RuntimeException("Failed to initialize memory store.");
		}

		return res;
	}
}

EnclaveStore::EnclaveStore(const MbedTlsObj::BigNumber & ringStart, const MbedTlsObj::BigNumber & ringEnd) :
	StoreBase(ringStart, ringEnd),
	m_memStore(InitializeMemStore())
{}

EnclaveStore::~EnclaveStore()
{
	ocall_decent_dht_mem_store_deinit(m_memStore);
}

bool EnclaveStore::IsResponsibleFor(const MbedTlsObj::BigNumber & key) const
{
	DhtStates::DhtLocalNodePtrType localNode = gs_state.GetDhtNode();
	return localNode ? localNode->IsResponsibleFor(key) : false;
}

std::vector<uint8_t> EnclaveStore::SaveDataFile(const MbedTlsObj::BigNumber& key, const std::vector<uint8_t>& data)
{
	using namespace Decent::Tools;

	const std::string keyStr = key.ToBigEndianHexStr();
	LOGI("DHT store: adding key to the index. %s", keyStr.c_str());
	LOGI("DHT store: writing value: %s", std::string(reinterpret_cast<const char*>(data.data()), data.size()).c_str());
	
	std::vector<uint8_t> meta;
	std::vector<uint8_t> mac;
	std::vector<uint8_t> sealedData = DataSealer::SealData(DataSealer::KeyPolicy::ByMrEnclave, mac, meta, data);
	
	{
		int memStoreRet = true;
		sgx_status_t sgxRet = ocall_decent_dht_mem_store_save(&memStoreRet, m_memStore, keyStr.c_str(), sealedData.data(), sealedData.size());
		if (sgxRet != SGX_SUCCESS)
		{
			throw RuntimeException(Sgx::ConstructSimpleErrorMsg(sgxRet, "ocall_decent_dht_mem_store_save"));
		}
		if (!memStoreRet)
		{
			throw RuntimeException("OCall ocall_decent_dht_mem_store_save failed.");
		}
	}

	return mac;
}

void EnclaveStore::DeleteDataFile(const MbedTlsObj::BigNumber& key)
{
	const std::string keyStr = key.ToBigEndianHexStr();

	{
		int memStoreRet = true;
		sgx_status_t sgxRet = ocall_decent_dht_mem_store_dele(&memStoreRet, m_memStore, keyStr.c_str());
		if (sgxRet != SGX_SUCCESS)
		{
			throw RuntimeException(Sgx::ConstructSimpleErrorMsg(sgxRet, "ocall_decent_dht_mem_store_save"));
		}
		if (!memStoreRet)
		{
			throw RuntimeException("OCall ocall_decent_dht_mem_store_save failed.");
		}
	}
}

std::vector<uint8_t> EnclaveStore::ReadDataFile(const MbedTlsObj::BigNumber& key, const std::vector<uint8_t>& tag)
{
	using namespace Decent::Tools;
	
	std::vector<uint8_t> sealedData;

	const std::string keyStr = key.ToBigEndianHexStr();

	{
		uint8_t* valPtr = nullptr;
		size_t valSize = 0;
		sgx_status_t sgxRet = ocall_decent_dht_mem_store_read(&valPtr, m_memStore, keyStr.c_str(), &valSize);
		if (sgxRet != SGX_SUCCESS)
		{
			throw RuntimeException(Sgx::ConstructSimpleErrorMsg(sgxRet, "ocall_decent_dht_mem_store_save"));
		}
		if (valPtr == nullptr)
		{
			throw RuntimeException("OCall ocall_decent_dht_mem_store_read failed.");
		}

		UntrustedBuffer uBuf(valPtr, valSize);

		sealedData = uBuf.Read();
	}

	std::vector<uint8_t> meta;
	std::vector<uint8_t> data;
	DataSealer::UnsealData(DataSealer::KeyPolicy::ByMrEnclave, sealedData, tag, meta, data);

	return data;
}

std::vector<uint8_t> EnclaveStore::MigrateOneDataFile(const MbedTlsObj::BigNumber& key, const std::vector<uint8_t>& tag)
{
	using namespace Decent::Tools;

	std::vector<uint8_t> sealedData;

	const std::string keyStr = key.ToBigEndianHexStr();

	{
		uint8_t* valPtr = nullptr;
		size_t valSize = 0;
		sgx_status_t sgxRet = ocall_decent_dht_mem_store_migrate_one(&valPtr, m_memStore, keyStr.c_str(), &valSize);
		if (sgxRet != SGX_SUCCESS)
		{
			throw RuntimeException(Sgx::ConstructSimpleErrorMsg(sgxRet, "ocall_decent_dht_mem_store_save"));
		}
		if (valPtr == nullptr)
		{
			throw RuntimeException("OCall ocall_decent_dht_mem_store_read failed.");
		}

		UntrustedBuffer uBuf(valPtr, valSize);

		sealedData = uBuf.Read();
	}

	std::vector<uint8_t> meta;
	std::vector<uint8_t> data;
	DataSealer::UnsealData(DataSealer::KeyPolicy::ByMrEnclave, sealedData, tag, meta, data);

	return data;
}

//#endif //ENCLAVE_PLATFORM_SGX
