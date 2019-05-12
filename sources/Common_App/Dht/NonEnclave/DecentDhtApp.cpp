#ifdef ENCLAVE_PLATFORM_NON_ENCLAVE

#include "DecentDhtApp.h"

#include <DecentApi/CommonApp/Base/EnclaveException.h>
#include <DecentApi/CommonApp/Threading/SingleTaskThreadPool.h>
#include <DecentApi/CommonApp/Threading/TaskSet.h>

#include "../../../Common/Dht/RequestCategory.h"

using namespace Decent::Net;
using namespace Decent::Dht;

extern "C" int ecall_decent_dht_init(uint64_t self_addr, int is_first_node, uint64_t ex_addr, size_t totalNode, size_t idx);
extern "C" void ecall_decent_dht_deinit();

extern "C" int ecall_decent_dht_proc_msg_from_dht(void* connection, void** prev_held_cnt);
extern "C" int ecall_decent_dht_proc_msg_from_store(void* connection);
extern "C" int ecall_decent_dht_proc_msg_from_app(void* connection);
extern "C" int ecall_decent_dht_forward_queue_worker();
extern "C" int ecall_decent_dht_reply_queue_worker();
extern "C" void ecall_decent_dht_terminate_workers();

Decent::Dht::DecentDhtApp::~DecentDhtApp()
{
	TerminateWorkers();
	ecall_decent_dht_deinit();
}

bool DecentDhtApp::ProcessMsgFromDht(ConnectionBase & connection, ConnectionBase*& freeHeldCnt)
{
	void*& prevHeldCntRef = reinterpret_cast<void*&>(freeHeldCnt);
	int retValue = ecall_decent_dht_proc_msg_from_dht(&connection, &prevHeldCntRef);

	return retValue;
}

bool DecentDhtApp::ProcessMsgFromStore(ConnectionBase & connection)
{
	int retValue = ecall_decent_dht_proc_msg_from_store(&connection);

	return retValue;
}

bool DecentDhtApp::ProcessMsgFromApp(ConnectionBase & connection)
{
	int retValue = ecall_decent_dht_proc_msg_from_app(&connection);

	return retValue;
}

void DecentDhtApp::QueryForwardWorker()
{
	int retVal = ecall_decent_dht_forward_queue_worker();
	if (!retVal)
	{
		throw RuntimeException("DecentDhtApp::QueryForwardWorker failed.");
	}
}

void DecentDhtApp::QueryReplyWorker()
{
	int retVal = ecall_decent_dht_reply_queue_worker();
	if (!retVal)
	{
		throw RuntimeException("DecentDhtApp::QueryReplyWorker failed.");
	}
}

void DecentDhtApp::TerminateWorkers()
{
	ecall_decent_dht_terminate_workers();
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
	else
	{
		return false;
	}
}

void DecentDhtApp::InitDhtNode(uint64_t selfAddr, uint64_t exNodeAddr, size_t totalNode, size_t idx)
{
	int retValue = ecall_decent_dht_init(selfAddr, exNodeAddr == 0, exNodeAddr, totalNode, idx);
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
