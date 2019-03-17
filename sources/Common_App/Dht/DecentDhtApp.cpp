#include "DecentDhtApp.h"

#include <DecentApi/Common/SGX/RuntimeError.h>
#include <DecentApi/CommonApp/Base/EnclaveException.h>

#include "Messages.h"

extern "C" sgx_status_t ecall_decent_dht_init(sgx_enclave_id_t eid, int* retval, uint64_t self_addr, int is_first_node, uint64_t ex_addr);
extern "C" sgx_status_t ecall_decent_dht_deinit(sgx_enclave_id_t eid);
extern "C" sgx_status_t ecall_decent_dht_proc_msg_from_dht(sgx_enclave_id_t eid, int* retval, void* connection);
extern "C" sgx_status_t ecall_decent_dht_proc_msg_from_store(sgx_enclave_id_t eid, int* retval, void* connection);
extern "C" sgx_status_t ecall_decent_dht_proc_msg_from_app(sgx_enclave_id_t eid, int* retval, void* connection);

using namespace Decent::Net;
using namespace Decent::Dht;

DecentDhtApp::DecentDhtApp(const std::string & enclavePath, const std::string & tokenPath, const std::string & wListKey, Connection & serverConn, uint64_t selfAddr, uint64_t exNodeAddr) :
	DecentApp(enclavePath, tokenPath, wListKey, serverConn)
{
	InitEnclave(selfAddr, exNodeAddr);
}

DecentDhtApp::DecentDhtApp(const fs::path & enclavePath, const fs::path & tokenPath, const std::string & wListKey, Connection & serverConn, uint64_t selfAddr, uint64_t exNodeAddr) :
	DecentApp(enclavePath, tokenPath, wListKey, serverConn)
{
	InitEnclave(selfAddr, exNodeAddr);
}

DecentDhtApp::DecentDhtApp(const std::string & enclavePath, const Decent::Tools::KnownFolderType tokenLocType, const std::string & tokenFileName, const std::string & wListKey, Connection & serverConn, uint64_t selfAddr, uint64_t exNodeAddr) :
	DecentApp(enclavePath, tokenLocType, tokenFileName, wListKey, serverConn)
{
	InitEnclave(selfAddr, exNodeAddr);
}

Decent::Dht::DecentDhtApp::~DecentDhtApp()
{
	ecall_decent_dht_deinit(GetEnclaveId());
}

bool DecentDhtApp::ProcessMsgFromDht(Decent::Net::Connection & connection)
{
	int retValue = false;

	sgx_status_t enclaveRet = ecall_decent_dht_proc_msg_from_dht(GetEnclaveId(), &retValue, &connection);
	DECENT_CHECK_SGX_STATUS_ERROR(enclaveRet, ecall_decent_dht_proc_msg_from_dht);

	return retValue;
}

bool DecentDhtApp::ProcessMsgFromStore(Decent::Net::Connection & connection)
{
	int retValue = false;

	sgx_status_t enclaveRet = ecall_decent_dht_proc_msg_from_store(GetEnclaveId(), &retValue, &connection);
	DECENT_CHECK_SGX_STATUS_ERROR(enclaveRet, ecall_decent_dht_proc_msg_from_store);

	return retValue;
}

bool DecentDhtApp::ProcessMsgFromApp(Decent::Net::Connection & connection)
{
	int retValue = false;

	sgx_status_t enclaveRet = ecall_decent_dht_proc_msg_from_app(GetEnclaveId(), &retValue, &connection);
	DECENT_CHECK_SGX_STATUS_ERROR(enclaveRet, ecall_decent_dht_proc_msg_from_app);

	return retValue;
}

bool DecentDhtApp::ProcessSmartMessage(const std::string & category, const Json::Value & jsonMsg, Decent::Net::Connection & connection)
{
	if (category == FromDht::sk_ValueCat)
	{
		return ProcessMsgFromDht(connection);
	}
	else if (category == FromStore::sk_ValueCat)
	{
		return ProcessMsgFromStore(connection);
	}
	else if (category == FromApp::sk_ValueCat)
	{
		return ProcessMsgFromApp(connection);
	}
	else
	{
		return Decent::RaSgx::DecentApp::ProcessSmartMessage(category, jsonMsg, connection);
	}
}

void DecentDhtApp::InitEnclave(uint64_t selfAddr, uint64_t exNodeAddr)
{
	int retValue = false;

	sgx_status_t enclaveRet = ecall_decent_dht_init(GetEnclaveId(), &retValue, selfAddr, exNodeAddr == 0, exNodeAddr);
	DECENT_CHECK_SGX_STATUS_ERROR(enclaveRet, ecall_decent_dht_init);
	DECENT_ASSERT_ENCLAVE_APP_RESULT(retValue, "Initialize DHT node.");
}
