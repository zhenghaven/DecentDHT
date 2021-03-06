#include "DhtServer.h"

#include <queue>

#include <cppcodec/base64_default_rfc4648.hpp>

#include <DecentApi/Common/Common.h>
#include <DecentApi/Common/make_unique.h>
#include <DecentApi/Common/GeneralKeyTypes.h>
#include <DecentApi/Common/Net/TlsCommLayer.h>
#include <DecentApi/Common/Net/ConnectionBase.h>
#include <DecentApi/Common/Net/SecureConnectionPoolBase.h>
#include <DecentApi/Common/MbedTls/BigNumber.h>
#include <DecentApi/Common/MbedTls/Drbg.h>

#include <DecentApi/CommonEnclave/Net/EnclaveCntTranslator.h>
#include <DecentApi/CommonEnclave/Ra/TlsConfigSameEnclave.h>

#include "../../Common/Dht/LocalNode.h"
#include "../../Common/Dht/FuncNums.h"
#include "../../Common/Dht/NodeBase.h"

#include "EnclaveStore.h"
#include "NodeConnector.h"
#include "ConnectionManager.h"
#include "DhtSecureConnectionMgr.h"
#include "DhtStatesSingleton.h"

using namespace Decent;
using namespace Decent::Net;
using namespace Decent::Dht;
using namespace Decent::MbedTlsObj;

namespace
{
	DhtStates& gs_state = Dht::GetDhtStatesSingleton();

	static char gsk_ack[] = "ACK";

	struct PendingQueryItem
	{
		std::unique_ptr<EnclaveCntTranslator> m_cnt;
		std::unique_ptr<TlsCommLayer> m_tls;
	};

	static std::mutex gs_clientPendingQueriesMutex;
	static std::map<uint64_t, std::unique_ptr<PendingQueryItem> > gs_clientPendingQueries;

	static bool TryGetQueriedAddrLocally(DhtStates::DhtLocalNodeType& localNode, const BigNumber& queriedId, uint64_t& resAddr)
	{
		if (localNode.IsResponsibleFor(queriedId))
		{
			//Query can be answered immediately.

			resAddr = localNode.GetAddress();

			return true;
		}
		else if (localNode.IsImmediatePredecessorOf(queriedId))
		{
			//Query can be answered immediately.

			DhtStates::DhtLocalNodeType::NodeBasePtr ImmeSucc = localNode.GetImmediateSuccessor();

			resAddr = ImmeSucc->GetAddress();

			return true;
		}

		return false;
	}

	static std::atomic<bool> gs_isForwardingTerminated = false;

	struct ForwardQueueItem
	{
		uint64_t m_reqId;
		uint8_t m_keyId[DhtStates::sk_keySizeByte];
		uint64_t m_reAddr;
	};

	static std::mutex gs_forwardQueueMutex;
	static std::queue<std::pair<uint64_t, std::unique_ptr<ForwardQueueItem> > > gs_forwardQueue;
	static std::condition_variable gs_forwardQueueSignal;

	static void ForwardQuery(const uint64_t& nextAddr, const ForwardQueueItem& item)
	{
		//PRINT_I("Forward query with ID %s.", item.m_uuid.c_str());
		CntPair peerCntPair = gs_state.GetConnectionMgr().GetNew(nextAddr, gs_state);

		peerCntPair.GetCommLayer().SendStruct(EncFunc::Dht::k_queryNonBlock);
		peerCntPair.GetCommLayer().SendStruct(item);
	}

	struct ReplyQueueItem
	{
		uint64_t m_reqId;
		uint64_t m_resAddr;
	};

	static std::mutex gs_replyQueueMutex;
	static std::queue<std::pair< uint64_t, std::unique_ptr<ReplyQueueItem> > > gs_replyQueue;
	static std::condition_variable gs_replyQueueSignal;

	static void ReplyQuery(const uint64_t& nextAddr, const ReplyQueueItem& item)
	{
		//PRINT_I("Reply query with ID %s.", item.m_uuid.c_str());
		CntPair peerCntPair = gs_state.GetConnectionMgr().GetNew(nextAddr, gs_state);

		peerCntPair.GetCommLayer().SendStruct(EncFunc::Dht::k_queryReply);
		peerCntPair.GetCommLayer().SendStruct(item);
	}
}

void Dht::DeUpdateFingerTable(Decent::Net::TlsCommLayer &tls)
{
	//LOGI("DHT Server: De-Updating FingerTable...");
	std::array<uint8_t, DhtStates::sk_keySizeByte> oldIdBin{};
	uint64_t i;

	tls.ReceiveRaw(oldIdBin.data(), oldIdBin.size()); //2. Receive old ID.

	DhtStates::DhtLocalNodeType::NodeBasePtr s = NodeConnector::ReceiveNode(tls); //3. Receive node.
	tls.ReceiveStruct(i); //3. Receive i. - Done!
	PRINT_I("De-updating finger table; i = %llu.", i);

	DhtStates::DhtLocalNodePtrType localNode = gs_state.GetDhtNode();

	localNode->DeUpdateFingerTable(ConstBigNumber(oldIdBin), s, i);

	tls.SendStruct(gsk_ack);
	//LOGI("");
}

void Dht::QueryNonBlock(Decent::Net::TlsCommLayer & tls)
{
	std::unique_ptr<ForwardQueueItem> forwardItem = Tools::make_unique<ForwardQueueItem>();

	tls.ReceiveStruct(*forwardItem);

	ConstBigNumber queriedId(forwardItem->m_keyId, sk_struct);

	DhtStates::DhtLocalNodePtrType localNode = gs_state.GetDhtNode();

	uint64_t resAddr = 0;
	if (TryGetQueriedAddrLocally(*localNode, queriedId, resAddr))
	{
		//Query can be answered immediately.

		std::unique_ptr<ReplyQueueItem> reItem = Tools::make_unique<ReplyQueueItem>();
		reItem->m_reqId = forwardItem->m_reqId;
		reItem->m_resAddr = resAddr;

		{
			std::unique_lock<std::mutex> replyQueueLock(gs_replyQueueMutex);
			gs_replyQueue.push(std::make_pair(forwardItem->m_reAddr, std::move(reItem)));
			replyQueueLock.unlock();
			gs_replyQueueSignal.notify_one();
		}

		return;
	}
	else
	{
		//Query has to be forwarded to other peer.
		
		DhtStates::DhtLocalNodeType::NodeBasePtr nextHop = localNode->GetNextHop(queriedId);

		{
			std::unique_lock<std::mutex> forwardQueueLock(gs_forwardQueueMutex);
			gs_forwardQueue.push(std::make_pair(nextHop->GetAddress(), std::move(forwardItem)));
			forwardQueueLock.unlock();
			gs_forwardQueueSignal.notify_one();
		}

		return;
	}
}

void Dht::QueryForwardWorker()
{
	std::queue<std::pair<uint64_t, std::unique_ptr<ForwardQueueItem> > > tmpQueue;
	//bool hasJob = false;
	//uint64_t addr = 0;
	//std::unique_ptr<ForwardQueueItem> item;

	while (!gs_isForwardingTerminated)
	{
		{
			std::unique_lock<std::mutex> forwardQueueLock(gs_forwardQueueMutex);
			if (gs_forwardQueue.size() == 0)
			{
				gs_forwardQueueSignal.wait(forwardQueueLock, []() {
					return gs_isForwardingTerminated || gs_forwardQueue.size() > 0;
				});
			}

			if (!gs_isForwardingTerminated && gs_forwardQueue.size() > 0)
			{
				tmpQueue.swap(gs_forwardQueue);
				//hasJob = true;
				//addr = gs_forwardQueue.front().first;
				//item = std::move(gs_forwardQueue.front().second);
				//gs_forwardQueue.pop();
			}
		}

		//if (hasJob)
		while (tmpQueue.size() > 0)
		{
			ForwardQuery(tmpQueue.front().first, *tmpQueue.front().second);
			tmpQueue.pop();
			//ForwardQuery(addr, *item);
			//hasJob = false;
		}
	}
}

void Dht::QueryReplyWorker()
{
	std::queue<std::pair< uint64_t, std::unique_ptr<ReplyQueueItem> > > tmpQueue;
	//bool hasJob = false;
	//uint64_t addr = 0;
	//std::unique_ptr<ReplyQueueItem> item;

	while (!gs_isForwardingTerminated)
	{
		{
			std::unique_lock<std::mutex> replyQueueLock(gs_replyQueueMutex);
			if (gs_replyQueue.size() == 0)
			{
				gs_replyQueueSignal.wait(replyQueueLock, []() {
					return gs_isForwardingTerminated || gs_replyQueue.size() > 0;
				});
			}

			if (!gs_isForwardingTerminated && gs_replyQueue.size() > 0)
			{
				tmpQueue.swap(gs_replyQueue);
				//hasJob = true;
				//addr = gs_replyQueue.front().first;
				//item = std::move(gs_replyQueue.front().second);
				//gs_replyQueue.pop();
			}
		}

		//if (hasJob)
		while (tmpQueue.size() > 0)
		{
			ReplyQuery(tmpQueue.front().first, *tmpQueue.front().second);
			tmpQueue.pop();
			//ReplyQuery(addr, *item);
			//hasJob = false;
		}
	}
}

void Dht::TerminateWorkers()
{
	gs_isForwardingTerminated = true;

	gs_forwardQueueSignal.notify_all();
	gs_replyQueueSignal.notify_all();
}

void Dht::QueryReply(Decent::Net::TlsCommLayer & tls, void*& heldCntPtr)
{
	ReplyQueueItem replyItem;

	tls.ReceiveStruct(replyItem);

	//PRINT_I("Received forwarded query reply with ID %s.", requestId.c_str());
	
	std::unique_ptr<PendingQueryItem> pendingItem;

	{
		std::unique_lock<std::mutex> clientPendingQueriesLock(gs_clientPendingQueriesMutex);
		auto it = gs_clientPendingQueries.find(replyItem.m_reqId);
		if (it != gs_clientPendingQueries.end())
		{
			pendingItem = std::move(it->second);

			gs_clientPendingQueries.erase(it);
		}
		else
		{
			LOGW("Pending request UUID is not found!");
			return;
		}
	}

	pendingItem->m_tls->SendStruct(replyItem.m_resAddr);
	heldCntPtr = pendingItem->m_cnt->GetPointer();
}

void Dht::UpdateFingerTable(Decent::Net::TlsCommLayer &tls)
{
	//LOGI("DHT Server: Updating FingerTable...");
	DhtStates::DhtLocalNodeType::NodeBasePtr s = NodeConnector::ReceiveNode(tls); //2. Receive node.

	uint64_t i;
	tls.ReceiveStruct(i); //3. Receive i. - Done!
	PRINT_I("Updating finger table; i = %llu.", i);

	DhtStates::DhtLocalNodePtrType localNode = gs_state.GetDhtNode();

	localNode->UpdateFingerTable(s, i);

	tls.SendStruct(gsk_ack);
	//LOGI("");
}

void Dht::GetImmediatePredecessor(Decent::Net::TlsCommLayer &tls)
{
	//LOGI("DHT Server: Getting Immediate Predecessor...");

	DhtStates::DhtLocalNodePtrType localNode = gs_state.GetDhtNode();

	NodeConnector::SendNode(tls, localNode->GetImmediatePredecessor()); //2. Send Node. - Done!
	//LOGI("");
}

void Dht::SetImmediatePredecessor(Decent::Net::TlsCommLayer &tls)
{
	//LOGI("DHT Server: Setting Immediate Predecessor...");

	DhtStates::DhtLocalNodePtrType localNode = gs_state.GetDhtNode();

	localNode->SetImmediatePredecessor(NodeConnector::ReceiveNode(tls)); //2. Receive Node. - Done!

	tls.SendStruct(gsk_ack);
	//LOGI("");
}

void Dht::GetImmediateSucessor(Decent::Net::TlsCommLayer &tls)
{
    //LOGI("DHT Server: Finding Immediate Successor...");

    DhtStates::DhtLocalNodePtrType localNode = gs_state.GetDhtNode();

	NodeConnector::SendNode(tls, localNode->GetImmediateSuccessor()); //2. Send Node. - Done!
	//LOGI("");
}

void Dht::GetNodeId(Decent::Net::TlsCommLayer &tls)
{
    //LOGI("DHT Server: Getting the NodeId...");
	DhtStates::DhtLocalNodePtrType localNode = gs_state.GetDhtNode();
    std::array<uint8_t, DhtStates::sk_keySizeByte> keyBin{};
    localNode->GetNodeId().ToBinary(keyBin);

    tls.SendRaw(keyBin.data(), keyBin.size()); //2. Sent nodeId ID

    //LOGI("Sent result ID: %s.", localNode->GetNodeId().ToBigEndianHexStr().c_str());
	//LOGI("");
}

void Dht::FindPredecessor(Decent::Net::TlsCommLayer &tls)
{
    //LOGI("DHT Server: Finding Predecessor...");
    std::array<uint8_t, DhtStates::sk_keySizeByte> keyBin{};
    tls.ReceiveRaw(keyBin.data(), keyBin.size()); //2. Received queried ID

    ConstBigNumber queriedId(keyBin);
    //LOGI("Recv queried ID: %s.", static_cast<const BigNumber&>(queriedId).ToBigEndianHexStr().c_str());

	DhtStates::DhtLocalNodePtrType localNode = gs_state.GetDhtNode();

	NodeConnector::SendNode(tls, localNode->FindPredecessor(queriedId)); //3. Send Node. - Done!
	//LOGI("");
}

void Dht::FindSuccessor(Decent::Net::TlsCommLayer &tls)
{
	//LOGI("DHT Server: Finding Successor...");
	std::array<uint8_t, DhtStates::sk_keySizeByte> keyBin{};
	tls.ReceiveRaw(keyBin.data(), keyBin.size()); //2. Received queried ID

	ConstBigNumber queriedId(keyBin);
	//LOGI("Recv queried ID: %s.", static_cast<const BigNumber&>(queriedId).ToBigEndianHexStr().c_str());

	DhtStates::DhtLocalNodePtrType localNode = gs_state.GetDhtNode();

	NodeConnector::SendNode(tls, localNode->FindSuccessor(queriedId)); //3. Send Node. - Done!
	//LOGI("");
}

void Dht::ProcessDhtQuery(Decent::Net::TlsCommLayer & tls, void*& heldCntPtr)
{
	using namespace EncFunc::Dht;

	//LOGI("DHT Server: Processing DHT queries...");

	NumType funcNum;
	tls.ReceiveStruct(funcNum); //1. Received function type.

	switch (funcNum)
	{

	case k_findSuccessor:
		FindSuccessor(tls);
		break;

	case k_findPredecessor:
        FindPredecessor(tls);
		break;

	case k_getImmediateSuc:
        GetImmediateSucessor(tls);
		break;

	case k_getNodeId:
	    GetNodeId(tls);
		break;

	case k_setImmediatePre:
		SetImmediatePredecessor(tls);
		break;

	case k_getImmediatePre:
		GetImmediatePredecessor(tls);
		break;

	case k_updFingerTable:
		UpdateFingerTable(tls);
		break;

	case k_dUpdFingerTable:
		DeUpdateFingerTable(tls);
		break;

	case k_queryNonBlock:
		QueryNonBlock(tls);
		break;

	case k_queryReply:
		QueryReply(tls, heldCntPtr);
		break;

	default:
		break;
	}
}

void Dht::ProcessStoreRequest(Decent::Net::TlsCommLayer & tls)
{
	using namespace EncFunc::Store;

	NumType funcNum;
	tls.ReceiveStruct(funcNum); //1. Received function type.

	switch (funcNum)
	{
	case k_getMigrateData:
		GetMigrateData(tls);
		break;

	case k_setMigrateData:
		SetMigrateData(tls);
		break;

	default:
		break;
	}
}

void Dht::GetMigrateData(Decent::Net::TlsCommLayer & tls)
{
	std::array<uint8_t, DhtStates::sk_keySizeByte> startKeyBin{};
	tls.ReceiveRaw(startKeyBin.data(), startKeyBin.size());
	ConstBigNumber start(startKeyBin);

	std::array<uint8_t, DhtStates::sk_keySizeByte> endKeyBin{};
	tls.ReceiveRaw(endKeyBin.data(), endKeyBin.size());
	ConstBigNumber end(endKeyBin);

	gs_state.GetDhtStore().SendMigratingData(
		[&tls](const void* buffer, const size_t size) -> void
	{
		tls.SendRaw(buffer, size);
	},
		[&tls](const BigNumber& key) -> void
	{
		std::array<uint8_t, DhtStates::sk_keySizeByte> keyBuf{};
		key.ToBinary(keyBuf);
		tls.SendRaw(keyBuf.data(), keyBuf.size());
	},
		start, end);
}

void Dht::SetMigrateData(Decent::Net::TlsCommLayer & tls)
{
	gs_state.GetDhtStore().RecvMigratingData(
		[&tls](void* buffer, const size_t size) -> void
	{
		tls.ReceiveRaw(buffer, size);
	},
		[&tls]() -> BigNumber
	{
		std::array<uint8_t, DhtStates::sk_keySizeByte> keyBuf{};
		tls.ReceiveRaw(keyBuf.data(), keyBuf.size());
		return BigNumber(keyBuf);
	});
}

void Dht::SetData(Decent::Net::TlsCommLayer & tls)
{
	std::array<uint8_t, DhtStates::sk_keySizeByte> keyBin{};
	tls.ReceiveRaw(keyBin.data(), keyBin.size());
	ConstBigNumber key(keyBin);

	//LOGI("Setting data for key %s.", key.Get().ToBigEndianHexStr().c_str());

	std::vector<uint8_t> buffer;
	tls.ReceiveMsg(buffer);

	gs_state.GetDhtStore().SetValue(key, std::move(buffer));

	tls.SendStruct(gsk_ack);
}

void Dht::GetData(Decent::Net::TlsCommLayer & tls)
{
	std::array<uint8_t, DhtStates::sk_keySizeByte> keyBin{};
	tls.ReceiveRaw(keyBin.data(), keyBin.size());
	ConstBigNumber key(keyBin);

	//LOGI("Getting data for key %s.", key.Get().ToBigEndianHexStr().c_str());

	std::vector<uint8_t> buffer = gs_state.GetDhtStore().GetValue(key);

	tls.SendMsg(buffer);
}

void Dht::DelData(Decent::Net::TlsCommLayer & tls)
{
	std::array<uint8_t, DhtStates::sk_keySizeByte> keyBin{};
	tls.ReceiveRaw(keyBin.data(), keyBin.size());
	ConstBigNumber key(keyBin);

	gs_state.GetDhtStore().DelValue(key);

	tls.SendStruct(gsk_ack);
}

namespace
{
	static std::shared_ptr<Ra::TlsConfigSameEnclave> GetClientTlsConfigDhtNode()
	{
		static std::shared_ptr<Ra::TlsConfigSameEnclave> tlsCfg = std::make_shared<Ra::TlsConfigSameEnclave>(gs_state, Ra::TlsConfig::Mode::ClientHasCert, nullptr);
		return tlsCfg;
	}

	static void MigrateDataFromPeer(EnclaveStore& dhtStore, const uint64_t & addr, const MbedTlsObj::BigNumber & start, const MbedTlsObj::BigNumber & end)
	{
		LOGI("Migrating data from peer...");
		using namespace EncFunc::Store;

		std::unique_ptr<ConnectionBase> connection = ConnectionManager::GetConnection2DecentStore(addr);
		Decent::Net::TlsCommLayer tls(*connection, GetClientTlsConfigDhtNode(), true, nullptr);

		tls.SendStruct(k_getMigrateData);         //1. Send function type

		std::array<uint8_t, DhtStates::sk_keySizeByte> keyBin{};

		start.ToBinary(keyBin);

		tls.SendRaw(keyBin.data(), keyBin.size()); //2. Send start key.
		end.ToBinary(keyBin);
		tls.SendRaw(keyBin.data(), keyBin.size()); //3. Send end key.

		dhtStore.RecvMigratingData(
			[&tls](void* buffer, const size_t size) -> void
		{
			tls.ReceiveRaw(buffer, size);
		},
			[&tls]() -> BigNumber
		{
			std::array<uint8_t, DhtStates::sk_keySizeByte> keyBuf{};
			tls.ReceiveRaw(keyBuf.data(), keyBuf.size());
			return BigNumber(keyBuf);
		}); //4. Receive data.
	}

	static void MigrateAllDataToPeer(EnclaveStore& dhtStore, const uint64_t & addr)
	{
		LOGI("Migrating data to peer...");
		using namespace EncFunc::Store;

		std::unique_ptr<ConnectionBase> connection = ConnectionManager::GetConnection2DecentStore(addr);
		Decent::Net::TlsCommLayer tls(*connection, GetClientTlsConfigDhtNode(), true, nullptr);

		tls.SendStruct(k_setMigrateData); //1. Send function type

		dhtStore.SendMigratingDataAll(
			[&tls](const void* buffer, const size_t size) -> void
		{
			tls.SendRaw(buffer, size);
		},
			[&tls](const BigNumber& key) -> void
		{
			std::array<uint8_t, DhtStates::sk_keySizeByte> keyBuf{};
			key.ToBinary(keyBuf);
			tls.SendRaw(keyBuf.data(), keyBuf.size());
		}); //2. Send data.
	}
}

void Dht::Init(uint64_t selfAddr, int isFirstNode, uint64_t exAddr, size_t totalNode, size_t idx)
{
	std::shared_ptr<DhtStates::DhtLocalNodeType::Pow2iArrayType> pow2iArray = std::make_shared<DhtStates::DhtLocalNodeType::Pow2iArrayType>();
	for (size_t i = 0; i < pow2iArray->size(); ++i)
	{
		(*pow2iArray)[i].SetBit(i, true);
	}

	std::array<uint8_t, DhtStates::sk_keySizeByte> filledArray;
	memset(filledArray.data(), 0xFF, filledArray.size());
	MbedTlsObj::ConstBigNumber largest(filledArray);

	BigNumber step = largest / totalNode;

	BigNumber selfId = step * idx;

	PRINT_I("Self Node ID: %s.", selfId.ToBigEndianHexStr().c_str());
	DhtStates::DhtLocalNodePtrType dhtNode = std::make_shared<DhtStates::DhtLocalNodeType>(selfId, selfAddr, 0, largest, pow2iArray);

	gs_state.GetDhtNode() = dhtNode;

	if (!isFirstNode)
	{
		NodeConnector nodeCnt(exAddr);
		dhtNode->Join(nodeCnt);

		uint64_t succAddr = dhtNode->GetImmediateSuccessor()->GetAddress();
		const BigNumber& predId = dhtNode->GetImmediatePredecessor()->GetNodeId();
		MigrateDataFromPeer(gs_state.GetDhtStore(), succAddr, selfId, predId);
	}
}

void Dht::DeInit()
{
	DhtStates::DhtLocalNodePtrType dhtNode = gs_state.GetDhtNode();
	uint64_t succAddr = dhtNode->GetImmediateSuccessor()->GetAddress();
	dhtNode->Leave();
	if (dhtNode->GetAddress() != succAddr)
	{
		MigrateAllDataToPeer(gs_state.GetDhtStore(), succAddr);
	}
}

bool Dht::ProcessAppRequest(Decent::Net::TlsCommLayer & tls, Net::EnclaveCntTranslator& cnt)
{
	using namespace EncFunc::App;

	NumType funcNum;
	tls.ReceiveStruct(funcNum); //1. Received function type.

	switch (funcNum)
	{
	case k_findSuccessor: return AppFindSuccessor(tls, cnt);

	case k_getData:
		GetData(tls);
		return false;

	case k_setData:
		SetData(tls);
		return false;

	case k_delData:
		DelData(tls);
		return false;

	default: return false;
	}
}

bool Dht::AppFindSuccessor(Decent::Net::TlsCommLayer & tls, Net::EnclaveCntTranslator& cnt)
{
	std::unique_ptr<ForwardQueueItem> queueItem = Tools::make_unique<ForwardQueueItem>();

	tls.ReceiveStruct(queueItem->m_keyId); //2. Received queried ID
	ConstBigNumber queriedId(queueItem->m_keyId, sk_struct);

	//LOGI("Recv app queried ID: %s.", static_cast<const BigNumber&>(queriedId).ToBigEndianHexStr().c_str());

	DhtStates::DhtLocalNodePtrType localNode = gs_state.GetDhtNode();
	
	uint64_t resAddr = 0;
	if (TryGetQueriedAddrLocally(*localNode, queriedId, resAddr))
	{
		//Query can be answered immediately.

		tls.SendStruct(resAddr);

		return false;
	}
	else
	{
		//Query should be forwarded to peer(s) and reply later.
		
		std::unique_ptr<PendingQueryItem> pendingItem = Tools::make_unique<PendingQueryItem>();

		pendingItem->m_cnt = Tools::make_unique<EnclaveCntTranslator>(std::move(cnt));
		pendingItem->m_tls = Tools::make_unique<TlsCommLayer>(std::move(tls));

		pendingItem->m_tls->SetConnectionPtr(*pendingItem->m_cnt);

		queueItem->m_reAddr = localNode->GetAddress();
		queueItem->m_reqId = reinterpret_cast<uint64_t>(pendingItem.get());

		//PRINT_I("Forward query with ID %s to peer.", requestId.c_str());
		
		{
			std::unique_lock<std::mutex> clientPendingQueriesLock(gs_clientPendingQueriesMutex);
			gs_clientPendingQueries.insert(
				std::make_pair(queueItem->m_reqId, std::move(pendingItem)));
		}

		DhtStates::DhtLocalNodeType::NodeBasePtr nextHop = localNode->GetNextHop(queriedId);

		{
			std::unique_lock<std::mutex> forwardQueueLock(gs_forwardQueueMutex);
			gs_forwardQueue.push(std::make_pair(nextHop->GetAddress(), std::move(queueItem)));
			forwardQueueLock.unlock();
			gs_forwardQueueSignal.notify_one();

			//NOTE! Don't use 'ConstBigNumber queriedId' after here!
		}

		return true;
	}
}
