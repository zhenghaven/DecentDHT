#ifdef ENCLAVE_PLATFORM_NON_ENCLAVE

#include "../UntrustedKeyValueStore.h"

#include <cstdint>

#include <DecentApi/Common/RuntimeException.h>

#ifdef __cplusplus
extern "C" {
#endif

	void* ocall_decent_dht_mem_store_init();
	void  ocall_decent_dht_mem_store_deinit(void* ptr);
	int ocall_decent_dht_mem_store_save(void* obj, const char* key, const uint8_t* val_ptr, const size_t val_size);
	uint8_t* ocall_decent_dht_mem_store_read(void* obj, const char* key, size_t* val_size);
	int ocall_decent_dht_mem_store_dele(void* obj, const char* key);
	uint8_t* ocall_decent_dht_mem_store_migrate_one(void* obj, const char* key, size_t* val_size);

#ifdef __cplusplus
}
#endif /* __cplusplus */

using namespace Decent::Dht;

namespace
{
	void* InitializeMemStore()
	{
		void* res = ocall_decent_dht_mem_store_init();

		if (res == nullptr)
		{
			throw Decent::RuntimeException("ocall_decent_dht_mem_store_init Failed");
		}
		return res;
	}
}

UntrustedKeyValueStore::UntrustedKeyValueStore() :
	m_ptr(InitializeMemStore())
{
}

UntrustedKeyValueStore::~UntrustedKeyValueStore()
{
	ocall_decent_dht_mem_store_deinit(m_ptr);
}

void UntrustedKeyValueStore::Save(const std::string& key, const std::vector<uint8_t>& value)
{
	int retVal = ocall_decent_dht_mem_store_save(m_ptr, key.c_str(), value.data(), value.size());
	if (!retVal)
	{
		throw Decent::RuntimeException("ocall_decent_dht_mem_store_save Failed");
	}
}

std::vector<uint8_t> UntrustedKeyValueStore::Read(const std::string & key)
{
	size_t valSize = 0;
	uint8_t* valPtr = ocall_decent_dht_mem_store_read(m_ptr, key.c_str(), &valSize);

	if (valPtr == nullptr)
	{
		throw Decent::RuntimeException("ocall_decent_dht_mem_store_read Failed");
	}

	std::vector<uint8_t> res(valPtr, valPtr + valSize);
	delete [] valPtr;

	return res;
}

void UntrustedKeyValueStore::Delete(const std::string & key)
{
	int retVal = ocall_decent_dht_mem_store_dele(m_ptr, key.c_str());
	if (!retVal)
	{
		throw Decent::RuntimeException("ocall_decent_dht_mem_store_dele Failed");
	}
}

std::vector<uint8_t> UntrustedKeyValueStore::MigrateOne(const std::string & key)
{
	size_t valSize = 0;
	uint8_t* valPtr = ocall_decent_dht_mem_store_migrate_one(m_ptr, key.c_str(), &valSize);

	if (valPtr == nullptr)
	{
		throw Decent::RuntimeException("ocall_decent_dht_mem_store_read Failed");
	}

	std::vector<uint8_t> res(valPtr, valPtr + valSize);
	delete[] valPtr;

	return res;
}

#endif //ENCLAVE_PLATFORM_NON_ENCLAVE
