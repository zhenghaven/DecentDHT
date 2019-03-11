#include "../ConnectionManager.h"

#include <DecentApi/Common/make_unique.h>
#include <DecentApi/CommonEnclave/SGX/OcallConnector.h>

using namespace Decent::Dht;
using namespace Decent::Net;

extern "C" sgx_status_t ocall_decent_dht_cnt_mgr_get_dht(void** out_cnt_ptr, uint64_t address);
extern "C" sgx_status_t ocall_decent_dht_cnt_mgr_get_store(void** out_cnt_ptr, uint64_t address);

std::unique_ptr<EnclaveNetConnector> ConnectionManager::GetConnection2DecentNode(uint64_t address)
{
	return Tools::make_unique<OcallConnector>(&ocall_decent_dht_cnt_mgr_get_dht, address);
}

std::unique_ptr<Decent::Net::EnclaveNetConnector> Decent::Dht::ConnectionManager::GetConnection2DecentStore(uint64_t address)
{
	return Tools::make_unique<OcallConnector>(&ocall_decent_dht_cnt_mgr_get_store, address);
}
