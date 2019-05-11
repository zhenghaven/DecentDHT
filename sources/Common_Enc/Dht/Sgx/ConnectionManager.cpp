#ifdef ENCLAVE_PLATFORM_SGX

#include "../ConnectionManager.h"

#include <DecentApi/Common/make_unique.h>
#include <DecentApi/CommonEnclave/Net/EnclaveConnectionOwner.h>

#include <sgx_edger8r.h>

extern "C" sgx_status_t ocall_decent_dht_cnt_mgr_get_dht(void** out_cnt_ptr, uint64_t address);
extern "C" sgx_status_t ocall_decent_dht_cnt_mgr_get_store(void** out_cnt_ptr, uint64_t address);

using namespace Decent::Dht;
using namespace Decent::Net;

std::unique_ptr<ConnectionBase> ConnectionManager::GetConnection2DecentNode(uint64_t address)
{
	return Tools::make_unique<EnclaveConnectionOwner>(EnclaveConnectionOwner::CntBuilder(SGX_SUCCESS, &ocall_decent_dht_cnt_mgr_get_dht, address));
}

std::unique_ptr<ConnectionBase> Decent::Dht::ConnectionManager::GetConnection2DecentStore(uint64_t address)
{
	return Tools::make_unique<EnclaveConnectionOwner>(EnclaveConnectionOwner::CntBuilder(SGX_SUCCESS, &ocall_decent_dht_cnt_mgr_get_store, address));
}

#endif //ENCLAVE_PLATFORM_SGX
