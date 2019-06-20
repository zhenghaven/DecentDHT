#pragma once

#include <DecentApi/DecentAppApp/DecentApp.h>

typedef struct _spid_t sgx_spid_t;

namespace Decent
{
	namespace Threading
	{
		class SingleTaskThreadPool;
	}

	namespace Ias
	{
		class Connector;
	}

	namespace Dht
	{
		class DecentDhtApp : public Decent::RaSgx::DecentApp
		{
		public:
			using DecentApp::DecentApp;

			virtual ~DecentDhtApp();

			virtual bool ProcessMsgFromDht(Decent::Net::ConnectionBase& connection, Net::ConnectionBase*& freeHeldCnt);

			virtual bool ProcessMsgFromStore(Decent::Net::ConnectionBase& connection);

			virtual bool ProcessMsgFromApp(Decent::Net::ConnectionBase& connection);

			virtual bool ProcessMsgFromUser(Decent::Net::ConnectionBase& connection);

			virtual void QueryForwardWorker();

			virtual void QueryReplyWorker();

			virtual void TerminateWorkers();

			virtual bool ProcessSmartMessage(const std::string& category, Net::ConnectionBase& connection, Net::ConnectionBase*& freeHeldCnt) override;

			void InitDhtNode(uint64_t selfAddr, uint64_t exNodeAddr, size_t totalNode, size_t idx, Ias::Connector* iasCntor, const sgx_spid_t& spid);

			void InitQueryWorkers(const size_t forwardWorkerNum, const size_t replyWorkerNum);

		private:
			std::unique_ptr<Threading::SingleTaskThreadPool> m_queryWorkerPool;
		};
	}
}

