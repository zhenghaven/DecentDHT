#include "DecentDhtApp.h"

#include <DecentApi/CommonApp/SGX/EnclaveRuntimeException.h>

#include "Messages.h"

extern "C" sgx_status_t ecall_decent_dht_proc_msg_from_dht(sgx_enclave_id_t eid, int* retval, void* connection);

using namespace Decent::Dht;

bool DecentDhtApp::ProcessMsgFromDht(Decent::Net::Connection & connection)
{
	int retValue = false;
	sgx_status_t enclaveRet = SGX_SUCCESS;

	enclaveRet = ecall_decent_dht_proc_msg_from_dht(GetEnclaveId(), &retValue, &connection);
	CHECK_SGX_ENCLAVE_RUNTIME_EXCEPTION(enclaveRet, ecall_decent_dht_loopup);

	return retValue;
}

bool DecentDhtApp::ProcessSmartMessage(const std::string & category, const Json::Value & jsonMsg, Decent::Net::Connection & connection)
{
	if (category == FromDht::sk_ValueCat)
	{
		return ProcessMsgFromDht(connection);
	}
	else
	{
		return Decent::RaSgx::DecentApp::ProcessSmartMessage(category, jsonMsg, connection);
	}
}
