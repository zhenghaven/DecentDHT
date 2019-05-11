#ifdef ENCLAVE_PLATFORM_SGX

#include "../../../Common/Dht/MemKeyValueStore.h"

using namespace Decent::Dht;

extern "C" void* ocall_decent_dht_mem_store_init()
{
	return new MemKeyValueStore();
}

extern "C" void  ocall_decent_dht_mem_store_deinit(void* ptr)
{
	MemKeyValueStore* objPtr = static_cast<MemKeyValueStore*>(ptr);
	delete objPtr;
}

extern "C" int ocall_decent_dht_mem_store_save(void* obj, const char* key, const uint8_t* val_ptr, const size_t val_size)
{
	if (!obj || !key || !val_ptr)
	{
		return false;
	}
	MemKeyValueStore* objPtr = static_cast<MemKeyValueStore*>(obj);

	try
	{
		MemKeyValueStore::ValueType val;
		val.first = val_size;
		val.second = std::make_unique<uint8_t[]>(val_size);
		std::copy(val_ptr, val_ptr + val_size, val.second.get());

		objPtr->Store(key, std::move(val));
	}
	catch (const std::exception&)
	{
		return false;
	}

	return true;
}

extern "C" uint8_t* ocall_decent_dht_mem_store_read(void* obj, const char* key, size_t* val_size)
{
	if (!obj || !key || !val_size)
	{
		return nullptr;
	}
	MemKeyValueStore* objPtr = static_cast<MemKeyValueStore*>(obj);

	try
	{
		MemKeyValueStore::ValueType val = objPtr->Read(key);

		*val_size = val.first;
		
		return val.second.release();
	}
	catch (const std::exception&)
	{
		return nullptr;
	}
}

extern "C" int ocall_decent_dht_mem_store_dele(void* obj, const char* key)
{
	if (!obj || !key)
	{
		return false;
	}
	MemKeyValueStore* objPtr = static_cast<MemKeyValueStore*>(obj);

	try
	{
		MemKeyValueStore::ValueType val = objPtr->Delete(key);

		return val.second.get() != nullptr;
	}
	catch (const std::exception&)
	{
		return false;
	}
}

extern "C" uint8_t* ocall_decent_dht_mem_store_migrate_one(void* obj, const char* key, size_t* val_size)
{
	if (!obj || !key || !val_size)
	{
		return nullptr;
	}
	MemKeyValueStore* objPtr = static_cast<MemKeyValueStore*>(obj);

	try
	{
		MemKeyValueStore::ValueType val = objPtr->Delete(key);

		*val_size = val.first;

		return val.second.release();
	}
	catch (const std::exception&)
	{
		return nullptr;
	}
}

#endif //ENCLAVE_PLATFORM_SGX
