#ifdef ENCLAVE_PLATFORM_SGX

#include "DecentDhtApp.h"

#include <DecentApi/Common/SGX/RuntimeError.h>
#include <DecentApi/CommonApp/Base/EnclaveException.h>
#include <DecentApi/CommonApp/Threading/SingleTaskThreadPool.h>
#include <DecentApi/CommonApp/Threading/TaskSet.h>

#include "../../../Common/Dht/RequestCategory.h"

extern "C" sgx_status_t ecall_decent_dht_init(sgx_enclave_id_t eid, int* retval, uint64_t self_addr, int is_first_node, uint64_t ex_addr, size_t totalNode, size_t idx, void* ias_cntor, const sgx_spid_t* spid, uint64_t enclave_Id);
extern "C" sgx_status_t ecall_decent_dht_deinit(sgx_enclave_id_t eid);

extern "C" sgx_status_t ecall_decent_dht_proc_msg_from_dht(sgx_enclave_id_t eid, int* retval, void* connection, void** prev_held_cnt);
extern "C" sgx_status_t ecall_decent_dht_proc_msg_from_store(sgx_enclave_id_t eid, int* retval, void* connection);
extern "C" sgx_status_t ecall_decent_dht_proc_msg_from_app(sgx_enclave_id_t eid, int* retval, void* connection);
extern "C" sgx_status_t ecall_decent_dht_proc_msg_from_user(sgx_enclave_id_t eid, int* retval, void* connection);

extern "C" sgx_status_t ecall_decent_dht_forward_queue_worker(sgx_enclave_id_t eid, int* retval);
extern "C" sgx_status_t ecall_decent_dht_reply_queue_worker(sgx_enclave_id_t eid, int* retval);
extern "C" sgx_status_t ecall_decent_dht_terminate_workers(sgx_enclave_id_t eid);

using namespace Decent::Net;
using namespace Decent::Dht;

Decent::Dht::DecentDhtApp::~DecentDhtApp()
{
	TerminateWorkers();
	ecall_decent_dht_deinit(GetEnclaveId());
}

bool DecentDhtApp::ProcessMsgFromDht(ConnectionBase & connection, ConnectionBase*& freeHeldCnt)
{
	int retValue = false;

	void*& prevHeldCntRef = reinterpret_cast<void*&>(freeHeldCnt);

	sgx_status_t enclaveRet = ecall_decent_dht_proc_msg_from_dht(GetEnclaveId(), &retValue, &connection, &prevHeldCntRef);
	DECENT_CHECK_SGX_STATUS_ERROR(enclaveRet, ecall_decent_dht_proc_msg_from_dht);

	return retValue;
}

bool DecentDhtApp::ProcessMsgFromStore(ConnectionBase & connection)
{
	int retValue = false;

	sgx_status_t enclaveRet = ecall_decent_dht_proc_msg_from_store(GetEnclaveId(), &retValue, &connection);
	DECENT_CHECK_SGX_STATUS_ERROR(enclaveRet, ecall_decent_dht_proc_msg_from_store);

	return retValue;
}

bool DecentDhtApp::ProcessMsgFromApp(ConnectionBase & connection)
{
	int retValue = false;

	sgx_status_t enclaveRet = ecall_decent_dht_proc_msg_from_app(GetEnclaveId(), &retValue, &connection);
	DECENT_CHECK_SGX_STATUS_ERROR(enclaveRet, ecall_decent_dht_proc_msg_from_app);

	return retValue;
}

bool DecentDhtApp::ProcessMsgFromUser(ConnectionBase & connection)
{
	int retValue = false;

	sgx_status_t enclaveRet = ecall_decent_dht_proc_msg_from_user(GetEnclaveId(), &retValue, &connection);
	DECENT_CHECK_SGX_STATUS_ERROR(enclaveRet, ecall_decent_dht_proc_msg_from_user);

	return retValue;
}

void DecentDhtApp::QueryForwardWorker()
{
	int retVal = false;

	sgx_status_t enclaveRet = ecall_decent_dht_forward_queue_worker(GetEnclaveId(), &retVal);
	DECENT_CHECK_SGX_STATUS_ERROR(enclaveRet, ecall_decent_dht_forward_queue_worker);
	if (!retVal)
	{
		throw RuntimeException("DecentDhtApp::QueryForwardWorker failed.");
	}
}

void DecentDhtApp::QueryReplyWorker()
{
	int retVal = false;

	sgx_status_t enclaveRet = ecall_decent_dht_reply_queue_worker(GetEnclaveId(), &retVal);
	DECENT_CHECK_SGX_STATUS_ERROR(enclaveRet, ecall_decent_dht_reply_queue_worker);
	if (!retVal)
	{
		throw RuntimeException("DecentDhtApp::QueryReplyWorker failed.");
	}
}

void DecentDhtApp::TerminateWorkers()
{
	ecall_decent_dht_terminate_workers(GetEnclaveId());
}

bool DecentDhtApp::ProcessSmartMessage(const std::string & category, ConnectionBase & connection, ConnectionBase*& freeHeldCnt)
{
	if (category == RequestCategory::sk_fromDht)
	{
		return ProcessMsgFromDht(connection, freeHeldCnt);
	}
	else if (category == RequestCategory::sk_fromStore)
	{
		return ProcessMsgFromStore(connection);
	}
	else if (category == RequestCategory::sk_fromApp)
	{
		return ProcessMsgFromApp(connection);
	}
	else if (category == RequestCategory::sk_fromUser)
	{
		return ProcessMsgFromUser(connection);
	}
	else
	{
		return Decent::RaSgx::DecentApp::ProcessSmartMessage(category, connection, freeHeldCnt);
	}
}

void DecentDhtApp::InitDhtNode(uint64_t selfAddr, uint64_t exNodeAddr, size_t totalNode, size_t idx, Decent::Ias::Connector* iasCntor, const sgx_spid_t& spid)
{
	int retValue = false;

	sgx_status_t enclaveRet = ecall_decent_dht_init(GetEnclaveId(), &retValue, selfAddr, exNodeAddr == 0, exNodeAddr, totalNode, idx, iasCntor, &spid, GetEnclaveId());
	DECENT_CHECK_SGX_STATUS_ERROR(enclaveRet, ecall_decent_dht_init);
	DECENT_ASSERT_ENCLAVE_APP_RESULT(retValue, "Initialize DHT node.");
}

void DecentDhtApp::InitQueryWorkers(const size_t forwardWorkerNum, const size_t replyWorkerNum)
{
	using namespace Decent::Threading;

	m_queryWorkerPool = std::make_unique<SingleTaskThreadPool>(nullptr);

	for (size_t i = 0; i < forwardWorkerNum; ++i)
	{
		std::unique_ptr<TaskSet> task = std::make_unique<TaskSet>(
			[this]() //Main task
		{
			this->QueryForwardWorker();
		},
			[this]() //Main task killer
		{
			this->TerminateWorkers();
		}
		);

		m_queryWorkerPool->AddTaskSet(task);
	}

	for (size_t i = 0; i < replyWorkerNum; ++i)
	{
		std::unique_ptr<TaskSet> task = std::make_unique<TaskSet>(
			[this]() //Main task
		{
			this->QueryReplyWorker();
		},
			[this]() //Main task killer
		{
			this->TerminateWorkers();
		}
		);

		m_queryWorkerPool->AddTaskSet(task);
	}
}

#endif // ENCLAVE_PLATFORM_SGX
