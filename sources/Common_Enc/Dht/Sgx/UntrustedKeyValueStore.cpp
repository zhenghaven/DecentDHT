#ifdef ENCLAVE_PLATFORM_SGX

#include "../UntrustedKeyValueStore.h"

#include <sgx_edger8r.h>

#include <DecentApi/Common/SGX/RuntimeError.h>
#include <DecentApi/CommonEnclave/Tools/UntrustedBuffer.h>

#ifdef __cplusplus
extern "C" {
#endif

	sgx_status_t SGX_CDECL ocall_decent_dht_mem_store_init(void** retval);
	sgx_status_t SGX_CDECL ocall_decent_dht_mem_store_deinit(void* ptr);
	sgx_status_t SGX_CDECL ocall_decent_dht_mem_store_save(int* retval, void* obj, const char* key, const uint8_t* val_ptr, size_t val_size);
	sgx_status_t SGX_CDECL ocall_decent_dht_mem_store_read(uint8_t** retval, void* obj, const char* key, size_t* val_size);
	sgx_status_t SGX_CDECL ocall_decent_dht_mem_store_dele(int* retval, void* obj, const char* key);
	sgx_status_t SGX_CDECL ocall_decent_dht_mem_store_migrate_one(uint8_t** retval, void* obj, const char* key, size_t* val_size);

#ifdef __cplusplus
}
#endif /* __cplusplus */

using namespace Decent::Dht;

namespace
{
	void* InitializeMemStore()
	{
		void* res = nullptr;

		DECENT_SGX_CALL_WITH_PTR_ERROR_1(ocall_decent_dht_mem_store_init, res);
		
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
	DECENT_SGX_CALL_WITH_INTBOOL_ERROR(ocall_decent_dht_mem_store_save, m_ptr, key.c_str(), value.data(), value.size());
}

std::vector<uint8_t> UntrustedKeyValueStore::Read(const std::string & key)
{
	uint8_t* valPtr = nullptr;
	size_t valSize = 0;

	DECENT_SGX_CALL_WITH_PTR_ERROR(ocall_decent_dht_mem_store_read, valPtr, m_ptr, key.c_str(), &valSize);

	using namespace Decent::Tools;
	UntrustedBuffer uBuf(valPtr, valSize);

	return uBuf.Read();
}

void UntrustedKeyValueStore::Delete(const std::string & key)
{
	DECENT_SGX_CALL_WITH_INTBOOL_ERROR(ocall_decent_dht_mem_store_dele, m_ptr, key.c_str());
}

std::vector<uint8_t> UntrustedKeyValueStore::MigrateOne(const std::string & key)
{
	uint8_t* valPtr = nullptr;
	size_t valSize = 0;

	DECENT_SGX_CALL_WITH_PTR_ERROR(ocall_decent_dht_mem_store_migrate_one, valPtr, m_ptr, key.c_str(), &valSize);

	using namespace Decent::Tools;
	UntrustedBuffer uBuf(valPtr, valSize);

	return uBuf.Read();
}

#endif //ENCLAVE_PLATFORM_SGX
