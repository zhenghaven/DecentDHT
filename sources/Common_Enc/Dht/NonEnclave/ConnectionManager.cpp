#ifdef ENCLAVE_PLATFORM_NON_ENCLAVE

#include "../ConnectionManager.h"

#include <DecentApi/Common/make_unique.h>
#include <DecentApi/Common/Net/ConnectionBase.h>
#include <DecentApi/Common/RuntimeException.h>

extern "C" void* ocall_decent_dht_cnt_mgr_get_dht(uint64_t address);
extern "C" void* ocall_decent_dht_cnt_mgr_get_store(uint64_t address);

using namespace Decent::Dht;
using namespace Decent::Net;

std::unique_ptr<ConnectionBase> ConnectionManager::GetConnection2DecentNode(uint64_t address)
{
	ConnectionBase* ptr = static_cast<ConnectionBase*>(ocall_decent_dht_cnt_mgr_get_dht(address));
	if (!ptr)
	{
		throw RuntimeException("Failed to establish connection to DHT node.");
	}
	return std::unique_ptr<ConnectionBase>(ptr);
}

std::unique_ptr<ConnectionBase> Decent::Dht::ConnectionManager::GetConnection2DecentStore(uint64_t address)
{
	ConnectionBase* ptr = static_cast<ConnectionBase*>(ocall_decent_dht_cnt_mgr_get_store(address));
	if (!ptr)
	{
		throw RuntimeException("Failed to establish connection to DHT store.");
	}
	return std::unique_ptr<ConnectionBase>(ptr);
}

#endif //ENCLAVE_PLATFORM_NON_ENCLAVE
