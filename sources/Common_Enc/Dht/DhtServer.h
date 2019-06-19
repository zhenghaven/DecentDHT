#pragma once

#include <cstdint>

#include <vector>

#include <DecentApi/Common/general_key_types.h>

#include "DhtStates.h"

namespace Decent
{
	namespace Net
	{
		class TlsCommLayer;
		class SecureCommLayer;
		class EnclaveCntTranslator;
	}

    namespace Dht
    {
		namespace AccessCtrl
		{
			class EntityItem;
			class AbAttributeList;
		}

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

		//(De-)Initialization functions:
		
		void Init(uint64_t selfAddr, int isFirstNode, uint64_t exAddr, size_t totalNode, size_t idx);

		void DeInit();

		//DHT node functions:
		
		void ProcessDhtQuery(Decent::Net::SecureCommLayer & secComm, void*& heldCntPtr);

		void GetNodeId(Decent::Net::SecureCommLayer &secComm);

		void FindSuccessor(Decent::Net::SecureCommLayer &secComm, const uint8_t(&keyId)[DhtStates::sk_keySizeByte]);

		void FindPredecessor(Decent::Net::SecureCommLayer &secComm, const uint8_t(&keyId)[DhtStates::sk_keySizeByte]);

		void GetImmediateSucessor(Decent::Net::SecureCommLayer &secComm);

		void GetImmediatePredecessor(Decent::Net::SecureCommLayer &secComm);

		void SetImmediatePredecessor(Decent::Net::SecureCommLayer &secComm, const uint8_t(&keyId)[DhtStates::sk_keySizeByte], uint64_t addr);

		void UpdateFingerTable(Decent::Net::SecureCommLayer &secComm, const uint8_t(&keyId)[DhtStates::sk_keySizeByte], uint64_t addr, uint64_t i);

		void DeUpdateFingerTable(Decent::Net::SecureCommLayer &secComm, const uint8_t(&oldIdBin)[DhtStates::sk_keySizeByte], const uint8_t(&keyId)[DhtStates::sk_keySizeByte], uint64_t addr, uint64_t i);

		void QueryNonBlock(const AddrForwardQueueItem& item);

		void QueryReply(const ReplyQueueItem& item, void*& heldCntPtr);

		void ListQueryNonBlock(const AttrListForwardQueueItem& item);

		void QueryForwardWorker();

		void QueryReplyWorker();

		void TerminateWorkers();

		//DHT Store functions:
		
		void ProcessStoreRequest(Decent::Net::SecureCommLayer & secComm);
		
		void GetMigrateData(Decent::Net::SecureCommLayer & secComm);

		void SetMigrateData(Decent::Net::SecureCommLayer & secComm);

		uint8_t InsertData(const uint8_t(&keyId)[DhtStates::sk_keySizeByte], 
			std::vector<uint8_t>::const_iterator metaSrcIt, std::vector<uint8_t>::const_iterator metaEnd, 
			std::vector<uint8_t>::const_iterator dataSrcIt, std::vector<uint8_t>::const_iterator dataEnd);

		uint8_t UpdateData(const uint8_t(&keyId)[DhtStates::sk_keySizeByte], const AccessCtrl::EntityItem& entity,
			std::vector<uint8_t>::const_iterator dataSrcIt, std::vector<uint8_t>::const_iterator dataEnd);

		uint8_t UpdateData(const uint8_t(&keyId)[DhtStates::sk_keySizeByte], const AccessCtrl::AbAttributeList& attList,
			AccessCtrl::AbAttributeList& neededAttList,
			std::vector<uint8_t>::const_iterator dataSrcIt, std::vector<uint8_t>::const_iterator dataEnd);

		uint8_t ReadData(const uint8_t(&keyId)[DhtStates::sk_keySizeByte], const AccessCtrl::EntityItem& entity,
			std::vector<uint8_t>& outData);

		uint8_t ReadData(const uint8_t(&keyId)[DhtStates::sk_keySizeByte], const AccessCtrl::AbAttributeList& attList,
			AccessCtrl::AbAttributeList& neededAttList,
			std::vector<uint8_t>& outData);

		uint8_t DelData(const uint8_t(&keyId)[DhtStates::sk_keySizeByte], const AccessCtrl::EntityItem& entity);

		uint8_t DelData(const uint8_t(&keyId)[DhtStates::sk_keySizeByte], const AccessCtrl::AbAttributeList& attList,
			AccessCtrl::AbAttributeList& neededAttList);

		//Requests from Apps:
		
		bool ProcessAppRequest(std::unique_ptr<Decent::Net::SecureCommLayer> & secCnt, Net::EnclaveCntTranslator& cnt, const std::vector<uint8_t>& encHash);

		bool AppFindSuccessor(std::unique_ptr<Decent::Net::SecureCommLayer> & secCnt, Net::EnclaveCntTranslator& cnt, const uint8_t(&keyId)[DhtStates::sk_keySizeByte]);

		//Request from Users (non-enclaves):
		
		bool ProcessUserRequest(std::unique_ptr<Decent::Net::TlsCommLayer> & tls, Net::EnclaveCntTranslator& cnt, const std::vector<uint8_t>& selfHash);

		bool AppFindSuccessor(std::unique_ptr<Decent::Net::TlsCommLayer> & tls, Net::EnclaveCntTranslator& cnt, const uint8_t(&keyId)[DhtStates::sk_keySizeByte]);

    }
}
