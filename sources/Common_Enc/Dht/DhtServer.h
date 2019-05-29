#pragma once

#include <cstdint>

#include <DecentApi/Common/general_key_types.h>

#include "DhtStates.h"

namespace Decent
{
	namespace Net
	{
		class TlsCommLayer;
		class EnclaveCntTranslator;
	}

    namespace Dht
    {

		struct AddrForwardQueueItem
		{
			uint64_t m_reqId;
			uint8_t m_keyId[DhtStates::sk_keySizeByte];
			uint64_t m_reAddr;
		};

		struct AttrListForwardQueueItem
		{
			uint64_t m_reqId;
			uint8_t m_keyId[DhtStates::sk_keySizeByte];
			general_256bit_hash m_userId;
			uint64_t m_reAddr;
		};

		struct ReplyQueueItem
		{
			uint64_t m_reqId;
			uint64_t m_resAddr;
		};

		//DHT node functions:
		
		void ProcessDhtQuery(Decent::Net::TlsCommLayer& tls, void*& heldCntPtr);

		void GetNodeId(Decent::Net::TlsCommLayer &tls);

		void FindSuccessor(Decent::Net::TlsCommLayer &tls, const uint8_t(&keyId)[DhtStates::sk_keySizeByte]);

		void FindPredecessor(Decent::Net::TlsCommLayer &tls, const uint8_t(&keyId)[DhtStates::sk_keySizeByte]);

		void GetImmediateSucessor(Decent::Net::TlsCommLayer &tls);

		void GetImmediatePredecessor(Decent::Net::TlsCommLayer &tls);

		void SetImmediatePredecessor(Decent::Net::TlsCommLayer &tls, const uint8_t(&keyId)[DhtStates::sk_keySizeByte], uint64_t addr);

		void UpdateFingerTable(Decent::Net::TlsCommLayer &tls, const uint8_t(&keyId)[DhtStates::sk_keySizeByte], uint64_t addr, uint64_t i);

		void DeUpdateFingerTable(Decent::Net::TlsCommLayer &tls, const uint8_t(&oldIdBin)[DhtStates::sk_keySizeByte], const uint8_t(&keyId)[DhtStates::sk_keySizeByte], uint64_t addr, uint64_t i);

		void QueryNonBlock(const AddrForwardQueueItem& item);

		void QueryReply(const ReplyQueueItem& item, void*& heldCntPtr);

		void ListQueryNonBlock(const AttrListForwardQueueItem& item);

		void QueryForwardWorker();

		void QueryReplyWorker();

		void TerminateWorkers();

		//DHT Store functions:
		
		void ProcessStoreRequest(Decent::Net::TlsCommLayer & tls);
		
		void GetMigrateData(Decent::Net::TlsCommLayer & tls);

		void SetMigrateData(Decent::Net::TlsCommLayer & tls);

		void SetData(Decent::Net::TlsCommLayer & tls);

		void GetData(Decent::Net::TlsCommLayer & tls);

		void DelData(Decent::Net::TlsCommLayer & tls);

		//(De-)Initialization functions:
		
		void Init(uint64_t selfAddr, int isFirstNode, uint64_t exAddr, size_t totalNode, size_t idx);

		void DeInit();

		//Requests from Apps:
		
		bool ProcessAppRequest(Decent::Net::TlsCommLayer & tls, Net::EnclaveCntTranslator& cnt);

		bool AppFindSuccessor(Decent::Net::TlsCommLayer &tls, Net::EnclaveCntTranslator& cnt);
    }
}
