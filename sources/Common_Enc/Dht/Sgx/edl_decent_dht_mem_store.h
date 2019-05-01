#pragma once
#ifndef EDL_DECENT_DHT_MEM_STORE_H
#define EDL_DECENT_DHT_MEM_STORE_H

#include <ctime>

#include <sgx_edger8r.h>

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

#endif // !EDL_DECENT_DHT_MEM_STORE_H
