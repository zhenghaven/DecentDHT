#include "DecentDhtApp.h"

#include <DecentApi/Common/SGX/RuntimeError.h>
#include <DecentApi/CommonApp/Base/EnclaveException.h>

#include "../../Common/Dht/RequestCategory.h"

extern "C" sgx_status_t ecall_decent_dht_init(sgx_enclave_id_t eid, int* retval, uint64_t self_addr, int is_first_node, uint64_t ex_addr, size_t totalNode, size_t idx);
extern "C" sgx_status_t ecall_decent_dht_deinit(sgx_enclave_id_t eid);
extern "C" sgx_status_t ecall_decent_dht_proc_msg_from_dht(sgx_enclave_id_t eid, int* retval, void* connection);
extern "C" sgx_status_t ecall_decent_dht_proc_msg_from_store(sgx_enclave_id_t eid, int* retval, void* connection);
extern "C" sgx_status_t ecall_decent_dht_proc_msg_from_app(sgx_enclave_id_t eid, int* retval, void* connection);

using namespace Decent::Net;
using namespace Decent::Dht;

Decent::Dht::DecentDhtApp::~DecentDhtApp()
{
	ecall_decent_dht_deinit(GetEnclaveId());
}

bool DecentDhtApp::ProcessMsgFromDht(Decent::Net::ConnectionBase & connection)
{
	int retValue = false;

	sgx_status_t enclaveRet = ecall_decent_dht_proc_msg_from_dht(GetEnclaveId(), &retValue, &connection);
	DECENT_CHECK_SGX_STATUS_ERROR(enclaveRet, ecall_decent_dht_proc_msg_from_dht);

	return retValue;
}

bool DecentDhtApp::ProcessMsgFromStore(Decent::Net::ConnectionBase & connection)
{
	int retValue = false;

	sgx_status_t enclaveRet = ecall_decent_dht_proc_msg_from_store(GetEnclaveId(), &retValue, &connection);
	DECENT_CHECK_SGX_STATUS_ERROR(enclaveRet, ecall_decent_dht_proc_msg_from_store);

	return retValue;
}

bool DecentDhtApp::ProcessMsgFromApp(Decent::Net::ConnectionBase & connection)
{
	int retValue = false;

	sgx_status_t enclaveRet = ecall_decent_dht_proc_msg_from_app(GetEnclaveId(), &retValue, &connection);
	DECENT_CHECK_SGX_STATUS_ERROR(enclaveRet, ecall_decent_dht_proc_msg_from_app);

	return retValue;
}

bool DecentDhtApp::ProcessSmartMessage(const std::string & category, Decent::Net::ConnectionBase & connection)
{
	if (category == RequestCategory::sk_fromDht)
	{
		return ProcessMsgFromDht(connection);
	}
	else if (category == RequestCategory::sk_fromStore)
	{
		return ProcessMsgFromStore(connection);
	}
	else if (category == RequestCategory::sk_fromApp)
	{
		return ProcessMsgFromApp(connection);
	}
	else
	{
		return Decent::RaSgx::DecentApp::ProcessSmartMessage(category, connection);
	}
}

void DecentDhtApp::InitDhtNode(uint64_t selfAddr, uint64_t exNodeAddr, size_t totalNode, size_t idx)
{
	int retValue = false;

	sgx_status_t enclaveRet = ecall_decent_dht_init(GetEnclaveId(), &retValue, selfAddr, exNodeAddr == 0, exNodeAddr, totalNode, idx);
	DECENT_CHECK_SGX_STATUS_ERROR(enclaveRet, ecall_decent_dht_init);
	DECENT_ASSERT_ENCLAVE_APP_RESULT(retValue, "Initialize DHT node.");
}
