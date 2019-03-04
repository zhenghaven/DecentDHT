#include "DecentDhtApp.h"

#include <DecentApi/CommonApp/SGX/EnclaveRuntimeException.h>

#include "../Common_App/Messages.h"

extern "C" sgx_status_t ecall_decent_dht_loopup(sgx_enclave_id_t eid, int* retval, void* connection);

using namespace Decent::Dht;

bool DecentDhtApp::ProcessMsgForDhtLookup(Decent::Net::Connection & connection)
{
	int retValue = false;
	sgx_status_t enclaveRet = SGX_SUCCESS;

	enclaveRet = ecall_decent_dht_loopup(GetEnclaveId(), &retValue, &connection);
	CHECK_SGX_ENCLAVE_RUNTIME_EXCEPTION(enclaveRet, ecall_decent_dht_loopup);

	return retValue;
}

bool DecentDhtApp::ProcessSmartMessage(const std::string & category, const Json::Value & jsonMsg, Decent::Net::Connection & connection)
{
	if (category == DhtLookup::sk_ValueCat)
	{
		return ProcessMsgForDhtLookup(connection);
	}
	else
	{
		return Decent::RaSgx::DecentApp::ProcessSmartMessage(category, jsonMsg, connection);
	}
}
