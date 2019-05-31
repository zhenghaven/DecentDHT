#include "DhtServer.h"

#include <queue>
#include <functional>

#include <cppcodec/base64_default_rfc4648.hpp>

#include <DecentApi/Common/Common.h>
#include <DecentApi/Common/make_unique.h>
#include <DecentApi/Common/GeneralKeyTypes.h>
#include <DecentApi/Common/MbedTls/Hasher.h>
#include <DecentApi/Common/MbedTls/BigNumber.h>
#include <DecentApi/Common/Net/TlsCommLayer.h>
#include <DecentApi/Common/Net/ConnectionBase.h>
#include <DecentApi/Common/Net/RpcParser.h>
#include <DecentApi/Common/Net/RpcWriter.h>

#include <DecentApi/CommonEnclave/Net/EnclaveCntTranslator.h>
#include <DecentApi/CommonEnclave/Ra/TlsConfigSameEnclave.h>

#include "../../Common/Dht/LocalNode.h"
#include "../../Common/Dht/FuncNums.h"
#include "../../Common/Dht/NodeBase.h"

#include "../../Common/Dht/AccessCtrl/FullPolicy.h"
#include "../../Common/Dht/AccessCtrl/AbAttributeList.h"

#include "EnclaveStore.h"
#include "NodeConnector.h"
#include "ConnectionManager.h"
#include "DhtSecureConnectionMgr.h"
#include "DhtStatesSingleton.h"

using namespace Decent;
using namespace Decent::Net;
using namespace Decent::Dht;
using namespace Decent::Dht::AccessCtrl;
using namespace Decent::MbedTlsObj;

namespace
{
	DhtStates& gs_state = Dht::GetDhtStatesSingleton();

	static char gsk_appKeyIdPrefix[] = "App::";
	static char gsk_atListKeyIdPrefix[] = "AttrList::";

	static void ReturnNode(SecureCommLayer & comm, DhtStates::DhtLocalNodeType::NodeBasePtr node)
	{
		constexpr size_t rpcSize = RpcWriter::CalcSizePrim<uint8_t[DhtStates::sk_keySizeByte]>() +
			RpcWriter::CalcSizePrim<uint64_t>();

		RpcWriter rpc(rpcSize, 2);

		auto keyBin = rpc.AddPrimitiveArg<uint8_t[DhtStates::sk_keySizeByte]>();
		node->GetNodeId().ToBinary(keyBin.Get(), sk_struct);

		rpc.AddPrimitiveArg<uint64_t>().Get() = node->GetAddress();

		comm.SendRpc(rpc);
	}

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

	static DhtStates::DhtLocalNodeType::NodeBasePtr GetAttrListQueryNextHop(DhtStates::DhtLocalNodeType& localNode, const BigNumber& queriedId)
	{
		if (localNode.IsResponsibleFor(queriedId))
		{
			//Query can be answered immediately.

			return nullptr;
		}
		else if (localNode.IsImmediatePredecessorOf(queriedId))
		{
			return localNode.GetImmediateSuccessor();
		}

		return localNode.GetNextHop(queriedId);;
	}

	static uint64_t CheckUserIdInAttrList(const BigNumber& queriedId, const general_256bit_hash& userId)
	{
		auto& dhtStore = gs_state.GetDhtStore();

		//dhtStore
		
		return 0; //false
	}
}

namespace
{
	static std::mutex gs_clientPendingQueriesMutex;
	static std::map<uint64_t, std::function<void*(const ReplyQueueItem&)> > gs_clientPendingQueries;

	static std::atomic<bool> gs_isForwardingTerminated = false;

	static std::mutex gs_forwardQueueMutex;
	static std::queue<std::pair<uint64_t, std::unique_ptr<AddrForwardQueueItem> > > gs_addrForwardQueue;
	static std::queue<std::pair<uint64_t, std::unique_ptr<AttrListForwardQueueItem> > > gs_attrListForwardQueue;
	static std::condition_variable gs_forwardQueueSignal;

	static void ForwardAddrQuery(const uint64_t& nextAddr, const AddrForwardQueueItem& item)
	{
		//PRINT_I("Forward query with ID %s.", item.m_uuid.c_str());
		CntPair peerCntPair = gs_state.GetConnectionMgr().GetNew(nextAddr, gs_state);

		constexpr size_t rpcSize = RpcWriter::CalcSizePrim<EncFunc::Dht::NumType>() +
			RpcWriter::CalcSizePrim<AddrForwardQueueItem>();

		RpcWriter rpc(rpcSize, 2);
		rpc.AddPrimitiveArg<EncFunc::Dht::NumType>().Get() = EncFunc::Dht::k_queryNonBlock;
		rpc.AddPrimitiveArg<AddrForwardQueueItem>().Get() = item;

		peerCntPair.GetCommLayer().SendRpc(rpc);
	}

	static void ForwardListQuery(const uint64_t& nextAddr, const AttrListForwardQueueItem& item)
	{
		CntPair peerCntPair = gs_state.GetConnectionMgr().GetNew(nextAddr, gs_state);

		constexpr size_t rpcSize = RpcWriter::CalcSizePrim<EncFunc::Dht::NumType>() +
			RpcWriter::CalcSizePrim<AttrListForwardQueueItem>();

		RpcWriter rpc(rpcSize, 2);
		rpc.AddPrimitiveArg<EncFunc::Dht::NumType>().Get() = EncFunc::Dht::k_listQueryNonBlock;
		rpc.AddPrimitiveArg<AttrListForwardQueueItem>().Get() = item;

		peerCntPair.GetCommLayer().SendRpc(rpc);
	}

	static std::mutex gs_replyQueueMutex;
	static std::queue<std::pair< uint64_t, std::unique_ptr<ReplyQueueItem> > > gs_replyQueue;
	static std::condition_variable gs_replyQueueSignal;

	static void ReplyQuery(const uint64_t& nextAddr, const ReplyQueueItem& item)
	{
		//PRINT_I("Reply query with ID %s.", item.m_uuid.c_str());
		CntPair peerCntPair = gs_state.GetConnectionMgr().GetNew(nextAddr, gs_state);

		constexpr size_t rpcSize = RpcWriter::CalcSizePrim<EncFunc::Dht::NumType>() +
			RpcWriter::CalcSizePrim<ReplyQueueItem>();

		RpcWriter rpc(rpcSize, 2);
		rpc.AddPrimitiveArg<EncFunc::Dht::NumType>().Get() = EncFunc::Dht::k_queryReply;
		rpc.AddPrimitiveArg<ReplyQueueItem>().Get() = item;

		peerCntPair.GetCommLayer().SendRpc(rpc);
	}

	static void PushToReplyQueue(uint64_t reAddr, std::unique_ptr<ReplyQueueItem> reItem)
	{
		std::unique_lock<std::mutex> replyQueueLock(gs_replyQueueMutex);
		gs_replyQueue.push(std::make_pair(reAddr, std::move(reItem)));
		replyQueueLock.unlock();
		gs_replyQueueSignal.notify_one();
	}
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

void Dht::QueryForwardWorker()
{
	std::queue<std::pair<uint64_t, std::unique_ptr<AddrForwardQueueItem> > > tmpAddrQueue;
	std::queue<std::pair<uint64_t, std::unique_ptr<AttrListForwardQueueItem> > > tmpListQueue;

	while (!gs_isForwardingTerminated)
	{
		{
			std::unique_lock<std::mutex> forwardQueueLock(gs_forwardQueueMutex);
			if (gs_addrForwardQueue.size() == 0 && gs_attrListForwardQueue.size() == 0)
			{
				gs_forwardQueueSignal.wait(forwardQueueLock, []() {
					return gs_isForwardingTerminated || gs_addrForwardQueue.size() > 0 || gs_attrListForwardQueue.size() > 0;
				});
			}

			if (!gs_isForwardingTerminated && gs_addrForwardQueue.size() > 0)
			{
				tmpAddrQueue.swap(gs_addrForwardQueue);
			}

			if (!gs_isForwardingTerminated && gs_attrListForwardQueue.size() > 0)
			{
				tmpListQueue.swap(gs_attrListForwardQueue);
			}
		}

		while (tmpAddrQueue.size() > 0)
		{
			ForwardAddrQuery(tmpAddrQueue.front().first, *tmpAddrQueue.front().second);
			tmpAddrQueue.pop();
		}

		while (tmpListQueue.size() > 0)
		{
			ForwardListQuery(tmpListQueue.front().first, *tmpListQueue.front().second);
			tmpListQueue.pop();
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

void Dht::GetNodeId(Decent::Net::TlsCommLayer &tls)
{
    //LOGI("DHT Server: Getting the NodeId...");
	DhtStates::DhtLocalNodePtrType localNode = gs_state.GetDhtNode();

	constexpr size_t rpcSize = RpcWriter::CalcSizePrim<uint8_t[DhtStates::sk_keySizeByte]>();

	RpcWriter rpc(rpcSize, 1);

	auto keyBin = rpc.AddPrimitiveArg<uint8_t[DhtStates::sk_keySizeByte]>();
	localNode->GetNodeId().ToBinary(keyBin.Get(), sk_struct);

	tls.SendRpc(rpc);
}

void Dht::FindSuccessor(Decent::Net::TlsCommLayer &tls, const uint8_t(&keyId)[DhtStates::sk_keySizeByte])
{
	ConstBigNumber queriedId(keyId);
	//LOGI("Recv queried ID: %s.", static_cast<const BigNumber&>(queriedId).ToBigEndianHexStr().c_str());

	DhtStates::DhtLocalNodePtrType localNode = gs_state.GetDhtNode();

	ReturnNode(tls, localNode->FindSuccessor(queriedId));
}

void Dht::FindPredecessor(Decent::Net::TlsCommLayer &tls, const uint8_t(&keyId)[DhtStates::sk_keySizeByte])
{
	ConstBigNumber queriedId(keyId);
    //LOGI("Recv queried ID: %s.", static_cast<const BigNumber&>(queriedId).ToBigEndianHexStr().c_str());

	DhtStates::DhtLocalNodePtrType localNode = gs_state.GetDhtNode();

	ReturnNode(tls, localNode->FindPredecessor(queriedId));
}

void Dht::GetImmediateSucessor(Decent::Net::TlsCommLayer &tls)
{
    //LOGI("DHT Server: Finding Immediate Successor...");

    DhtStates::DhtLocalNodePtrType localNode = gs_state.GetDhtNode();

	ReturnNode(tls, localNode->GetImmediateSuccessor());
}

void Dht::GetImmediatePredecessor(Decent::Net::TlsCommLayer &tls)
{
	//LOGI("DHT Server: Getting Immediate Predecessor...");

	DhtStates::DhtLocalNodePtrType localNode = gs_state.GetDhtNode();

	ReturnNode(tls, localNode->GetImmediatePredecessor());
}

void Dht::SetImmediatePredecessor(Decent::Net::TlsCommLayer &tls, const uint8_t(&keyId)[DhtStates::sk_keySizeByte], uint64_t addr)
{
	//LOGI("DHT Server: Setting Immediate Predecessor...");

	DhtStates::DhtLocalNodePtrType localNode = gs_state.GetDhtNode();

	localNode->SetImmediatePredecessor(std::make_shared<NodeConnector>(addr, BigNumber(keyId, true))); //2. Receive Node. - Done!

	tls.SendRpc(RpcWriter(0, 0));
}

void Dht::UpdateFingerTable(Decent::Net::TlsCommLayer &tls, const uint8_t(&keyId)[DhtStates::sk_keySizeByte], uint64_t addr, uint64_t i)
{
	PRINT_I("Updating finger table; i = %llu.", i);

	DhtStates::DhtLocalNodeType::NodeBasePtr s = std::make_shared<NodeConnector>(addr, BigNumber(keyId, true));

	DhtStates::DhtLocalNodePtrType localNode = gs_state.GetDhtNode();

	localNode->UpdateFingerTable(s, i);

	tls.SendRpc(RpcWriter(0, 0));
}

void Dht::DeUpdateFingerTable(Decent::Net::TlsCommLayer &tls, const uint8_t(&oldId)[DhtStates::sk_keySizeByte], const uint8_t(&keyId)[DhtStates::sk_keySizeByte], uint64_t addr, uint64_t i)
{
	PRINT_I("De-updating finger table; i = %llu.", i);

	DhtStates::DhtLocalNodeType::NodeBasePtr s = std::make_shared<NodeConnector>(addr, BigNumber(keyId, true));

	DhtStates::DhtLocalNodePtrType localNode = gs_state.GetDhtNode();

	localNode->DeUpdateFingerTable(ConstBigNumber(oldId), s, i);

	tls.SendRpc(RpcWriter(0, 0));
}

void Dht::QueryNonBlock(const AddrForwardQueueItem& item)
{
	std::unique_ptr<AddrForwardQueueItem> forwardItem = Tools::make_unique<AddrForwardQueueItem>(item);

	ConstBigNumber queriedId(forwardItem->m_keyId);

	DhtStates::DhtLocalNodePtrType localNode = gs_state.GetDhtNode();

	uint64_t resAddr = 0;
	if (TryGetQueriedAddrLocally(*localNode, queriedId, resAddr))
	{
		//Query can be answered immediately.

		std::unique_ptr<ReplyQueueItem> reItem = Tools::make_unique<ReplyQueueItem>();
		reItem->m_reqId = forwardItem->m_reqId;
		reItem->m_resAddr = resAddr;

		PushToReplyQueue(forwardItem->m_reAddr, std::move(reItem));

		return;
	}
	else
	{
		//Query has to be forwarded to other peer.

		DhtStates::DhtLocalNodeType::NodeBasePtr nextHop = localNode->GetNextHop(queriedId);

		{
			std::unique_lock<std::mutex> forwardQueueLock(gs_forwardQueueMutex);
			gs_addrForwardQueue.push(std::make_pair(nextHop->GetAddress(), std::move(forwardItem)));
			forwardQueueLock.unlock();
			gs_forwardQueueSignal.notify_one();
		}

		return;
	}
}

void Dht::QueryReply(const ReplyQueueItem& item, void*& heldCntPtr)
{
	//PRINT_I("Received forwarded query reply with ID %s.", requestId.c_str());

	std::function<void*(const ReplyQueueItem&)> callBackFunc;

	{
		std::unique_lock<std::mutex> clientPendingQueriesLock(gs_clientPendingQueriesMutex);
		auto it = gs_clientPendingQueries.find(item.m_reqId);
		if (it != gs_clientPendingQueries.end())
		{
			callBackFunc = std::move(it->second);

			gs_clientPendingQueries.erase(it);
		}
		else
		{
			LOGW("Pending request UUID is not found!");
			return;
		}
	}

	heldCntPtr = callBackFunc(item);
}

void Dht::ListQueryNonBlock(const AttrListForwardQueueItem & item)
{
	std::unique_ptr<AttrListForwardQueueItem> forwardItem = Tools::make_unique<AttrListForwardQueueItem>(item);

	ConstBigNumber queriedId(forwardItem->m_keyId);

	DhtStates::DhtLocalNodePtrType localNode = gs_state.GetDhtNode();

	DhtStates::DhtLocalNodeType::NodeBasePtr nextHop = GetAttrListQueryNextHop(*localNode, queriedId);
	if (!nextHop)
	{
		//Answer immediately

		std::unique_ptr<ReplyQueueItem> reItem = Tools::make_unique<ReplyQueueItem>();

		reItem->m_reqId = forwardItem->m_reqId;
		reItem->m_resAddr = CheckUserIdInAttrList(queriedId, forwardItem->m_userId);

		PushToReplyQueue(forwardItem->m_reAddr, std::move(reItem));
	}
	else
	{
		std::unique_lock<std::mutex> forwardQueueLock(gs_forwardQueueMutex);
		gs_attrListForwardQueue.push(std::make_pair(nextHop->GetAddress(), std::move(forwardItem)));
		forwardQueueLock.unlock();
		gs_forwardQueueSignal.notify_one();
	}
}

void Dht::ProcessDhtQuery(Decent::Net::TlsCommLayer & tls, void*& heldCntPtr)
{
	using namespace EncFunc::Dht;

	//LOGI("DHT Server: Processing DHT queries...");

	RpcParser rpc(tls.ReceiveBinary());
	const NumType& funcNum = rpc.GetPrimitiveArg<NumType>();

	switch (funcNum)
	{

	case k_getNodeId:
		if (rpc.GetArgCount() == 1)
		{
			GetNodeId(tls);
		}
		else
		{
			throw RuntimeException("Number of arguments for function k_getNodeId doesn't match.");
		}
		break;

	case k_findSuccessor:
		if (rpc.GetArgCount() == 2)
		{
			const auto& keyId = rpc.GetPrimitiveArg<uint8_t[DhtStates::sk_keySizeByte]>();
			FindSuccessor(tls, keyId);
		}
		else
		{
			throw RuntimeException("Number of arguments for function k_findSuccessor doesn't match.");
		}
		break;

	case k_findPredecessor:
		if (rpc.GetArgCount() == 2)
		{
			const auto& keyId = rpc.GetPrimitiveArg<uint8_t[DhtStates::sk_keySizeByte]>();
			FindPredecessor(tls, keyId);
		}
		else
		{
			throw RuntimeException("Number of arguments for function k_findPredecessor doesn't match.");
		}
		break;

	case k_getImmediateSuc:
		if (rpc.GetArgCount() == 1)
		{
			GetImmediateSucessor(tls);
		}
		else
		{
			throw RuntimeException("Number of arguments for function k_getImmediateSuc doesn't match.");
		}
		break;

	case k_getImmediatePre:
		if (rpc.GetArgCount() == 1)
		{
			GetImmediatePredecessor(tls);
		}
		else
		{
			throw RuntimeException("Number of arguments for function k_getImmediatePre doesn't match.");
		}
		break;

	case k_setImmediatePre:
		if (rpc.GetArgCount() == 3)
		{
			const auto& keyId = rpc.GetPrimitiveArg<uint8_t[DhtStates::sk_keySizeByte]>();
			const auto& addr = rpc.GetPrimitiveArg<uint64_t>();
			SetImmediatePredecessor(tls, keyId, addr);
		}
		else
		{
			throw RuntimeException("Number of arguments for function k_setImmediatePre doesn't match.");
		}
		break;

	case k_updFingerTable:
		if (rpc.GetArgCount() == 4)
		{
			const auto& keyId = rpc.GetPrimitiveArg<uint8_t[DhtStates::sk_keySizeByte]>();
			const auto& addr = rpc.GetPrimitiveArg<uint64_t>();
			const auto& i = rpc.GetPrimitiveArg<uint64_t>();
			UpdateFingerTable(tls, keyId, addr, i);
		}
		else
		{
			throw RuntimeException("Number of arguments for function k_updFingerTable doesn't match.");
		}
		break;

	case k_dUpdFingerTable:
		if (rpc.GetArgCount() == 5)
		{
			const auto& oldId = rpc.GetPrimitiveArg<uint8_t[DhtStates::sk_keySizeByte]>();
			const auto& keyId = rpc.GetPrimitiveArg<uint8_t[DhtStates::sk_keySizeByte]>();
			const auto& addr = rpc.GetPrimitiveArg<uint64_t>();
			const auto& i = rpc.GetPrimitiveArg<uint64_t>();
			DeUpdateFingerTable(tls, oldId, keyId, addr, i);
		}
		else
		{
			throw RuntimeException("Number of arguments for function k_dUpdFingerTable doesn't match.");
		}
		break;

	case k_queryNonBlock:
		if (rpc.GetArgCount() == 2)
		{
			const auto& item = rpc.GetPrimitiveArg<AddrForwardQueueItem>();
			QueryNonBlock(item);
		}
		else
		{
			throw RuntimeException("Number of arguments for function k_queryNonBlock doesn't match.");
		}
		break;

	case k_queryReply:
		if (rpc.GetArgCount() == 2)
		{
			const auto& item = rpc.GetPrimitiveArg<ReplyQueueItem>();
			QueryReply(item, heldCntPtr);
		}
		else
		{
			throw RuntimeException("Number of arguments for function k_queryReply doesn't match.");
		}
		break;

	case k_listQueryNonBlock:
		if (rpc.GetArgCount() == 2)
		{
			const auto& item = rpc.GetPrimitiveArg<AttrListForwardQueueItem>();
			ListQueryNonBlock(item);
		}
		else
		{
			throw RuntimeException("Number of arguments for function k_queryNonBlock doesn't match.");
		}
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

uint8_t Dht::InsertData(const uint8_t(&keyId)[DhtStates::sk_keySizeByte],
	std::vector<uint8_t>::const_iterator metaSrcIt, std::vector<uint8_t>::const_iterator metaEnd,
	std::vector<uint8_t>::const_iterator dataSrcIt, std::vector<uint8_t>::const_iterator dataEnd)
{
	ConstBigNumber key(keyId);

	try
	{
		gs_state.GetDhtStore().SetValue(key, std::vector<uint8_t>(metaSrcIt, metaEnd), std::vector<uint8_t>(dataSrcIt, dataEnd), false);
	}
	catch (const DataAlreadyExist&)
	{
		return EncFunc::FileOpRet::k_denied;
	}

	return EncFunc::FileOpRet::k_success;
}

uint8_t Dht::UpdateData(const uint8_t(&keyId)[DhtStates::sk_keySizeByte],
	const AccessCtrl::EntityItem & entity,
	std::vector<uint8_t>::const_iterator dataSrcIt, std::vector<uint8_t>::const_iterator dataEnd)
{
	ConstBigNumber key(keyId);

	std::vector<uint8_t> meta;
	try
	{
		std::vector<uint8_t> data = gs_state.GetDhtStore().GetValue(key, meta);

		auto it = meta.cbegin();
		FullPolicy fullPolicy(it, meta.cend());

		if (!fullPolicy.GetEnclavePolicy().ExamineWrite(entity))
		{
			return EncFunc::FileOpRet::k_denied;
		}
	}
	catch (const DataNotExist&)
	{
		return EncFunc::FileOpRet::k_nonExist;
	}

	gs_state.GetDhtStore().SetValue(key, meta, std::vector<uint8_t>(dataSrcIt, dataEnd));

	return EncFunc::FileOpRet::k_success;
}

uint8_t Dht::UpdateData(const uint8_t(&keyId)[DhtStates::sk_keySizeByte],
	const AccessCtrl::AbAttributeList & attList, AccessCtrl::AbAttributeList & neededAttList,
	std::vector<uint8_t>::const_iterator dataSrcIt, std::vector<uint8_t>::const_iterator dataEnd)
{
	ConstBigNumber key(keyId);

	std::vector<uint8_t> meta;
	try
	{
		std::vector<uint8_t> data = gs_state.GetDhtStore().GetValue(key, meta);

		auto it = meta.cbegin();
		FullPolicy fullPolicy(it, meta.cend());

		if (!fullPolicy.GetClientPolicy().ExamineWrite(attList))
		{
			fullPolicy.GetClientPolicy().GetWriteRelatedAttributes(neededAttList);
			neededAttList -= attList;

			return EncFunc::FileOpRet::k_denied;
		}
	}
	catch (const DataNotExist&)
	{
		return EncFunc::FileOpRet::k_nonExist;
	}

	gs_state.GetDhtStore().SetValue(key, meta, std::vector<uint8_t>(dataSrcIt, dataEnd));

	return EncFunc::FileOpRet::k_success;
}

uint8_t Dht::ReadData(const uint8_t(&keyId)[DhtStates::sk_keySizeByte],
	const AccessCtrl::EntityItem & entity, std::vector<uint8_t>& outData)
{
	ConstBigNumber key(keyId);

	std::vector<uint8_t> meta;
	try
	{
		std::vector<uint8_t> data = gs_state.GetDhtStore().GetValue(key, meta);

		auto it = meta.cbegin();
		FullPolicy fullPolicy(it, meta.cend());

		if (!fullPolicy.GetEnclavePolicy().ExamineRead(entity))
		{
			return EncFunc::FileOpRet::k_denied;
		}
		else
		{
			outData.swap(data);
			return EncFunc::FileOpRet::k_success;
		}
	}
	catch (const DataNotExist&)
	{
		return EncFunc::FileOpRet::k_nonExist;
	}
}

uint8_t Dht::ReadData(const uint8_t(&keyId)[DhtStates::sk_keySizeByte],
	const AccessCtrl::AbAttributeList & attList, AccessCtrl::AbAttributeList & neededAttList,
	std::vector<uint8_t>& outData)
{
	ConstBigNumber key(keyId);

	std::vector<uint8_t> meta;
	try
	{
		std::vector<uint8_t> data = gs_state.GetDhtStore().GetValue(key, meta);

		auto it = meta.cbegin();
		FullPolicy fullPolicy(it, meta.cend());

		if (!fullPolicy.GetClientPolicy().ExamineRead(attList))
		{
			fullPolicy.GetClientPolicy().GetReadRelatedAttributes(neededAttList);
			neededAttList -= attList;

			return EncFunc::FileOpRet::k_denied;
		}
		else
		{
			outData.swap(data);
			return EncFunc::FileOpRet::k_success;
		}
	}
	catch (const DataNotExist&)
	{
		return EncFunc::FileOpRet::k_nonExist;
	}
}

uint8_t Dht::DelData(const uint8_t(&keyId)[DhtStates::sk_keySizeByte],
	const AccessCtrl::EntityItem & entity)
{
	ConstBigNumber key(keyId);

	std::vector<uint8_t> meta;
	try
	{
		std::vector<uint8_t> data = gs_state.GetDhtStore().GetValue(key, meta);

		auto it = meta.cbegin();
		FullPolicy fullPolicy(it, meta.cend());

		if (!fullPolicy.GetEnclavePolicy().ExamineWrite(entity))
		{
			return EncFunc::FileOpRet::k_denied;
		}
		else
		{
			gs_state.GetDhtStore().DelValue(key);
			return EncFunc::FileOpRet::k_success;
		}
	}
	catch (const DataNotExist&)
	{
		return EncFunc::FileOpRet::k_nonExist;
	}
}

uint8_t Dht::DelData(const uint8_t(&keyId)[DhtStates::sk_keySizeByte],
	const AccessCtrl::AbAttributeList & attList, AccessCtrl::AbAttributeList & neededAttList)
{
	ConstBigNumber key(keyId);

	std::vector<uint8_t> meta;
	try
	{
		std::vector<uint8_t> data = gs_state.GetDhtStore().GetValue(key, meta);

		auto it = meta.cbegin();
		FullPolicy fullPolicy(it, meta.cend());

		if (!fullPolicy.GetClientPolicy().ExamineWrite(attList))
		{
			fullPolicy.GetClientPolicy().GetWriteRelatedAttributes(neededAttList);
			neededAttList -= attList;

			return EncFunc::FileOpRet::k_denied;
		}
		else
		{
			gs_state.GetDhtStore().DelValue(key);
			return EncFunc::FileOpRet::k_success;
		}
	}
	catch (const DataNotExist&)
	{
		return EncFunc::FileOpRet::k_nonExist;
	}
}

bool Dht::ProcessAppRequest(Decent::Net::TlsCommLayer & tls, Net::EnclaveCntTranslator& cnt, const std::vector<uint8_t>& encHash)
{
	using namespace EncFunc::App;

	RpcParser rpc(tls.ReceiveBinary());

	const auto& funcNum = rpc.GetPrimitiveArg<NumType>(); //Arg 1.

	switch (funcNum)
	{
	case k_findSuccessor:
		if (rpc.GetArgCount() == 2)
		{
			const auto& keyId = rpc.GetPrimitiveArg<uint8_t[DhtStates::sk_keySizeByte]>(); //Arg 2.

			uint8_t appKeyId[DhtStates::sk_keySizeByte] = { 0 };
			Hasher::ArrayBatchedCalc<HashType::SHA256>(appKeyId, gsk_appKeyIdPrefix, keyId);

			return AppFindSuccessor(tls, cnt, appKeyId);
		}
		else
		{
			throw RuntimeException("Number of arguments for function k_findSuccessor doesn't match.");
		}

	case k_readData:
		if (rpc.GetArgCount() == 2)
		{
			const auto& keyId = rpc.GetPrimitiveArg<uint8_t[DhtStates::sk_keySizeByte]>(); //Arg 2.

			uint8_t appKeyId[DhtStates::sk_keySizeByte] = { 0 };
			Hasher::ArrayBatchedCalc<HashType::SHA256>(appKeyId, gsk_appKeyIdPrefix, keyId);

			if (encHash.size() != sizeof(general_256bit_hash))
			{
				throw RuntimeException("Feature not implemented yet. (Enclave hash is not 256 bit)");
			}

			general_256bit_hash tmpEncHash;
			std::copy(encHash.begin(), encHash.end(), std::begin(tmpEncHash));

			AccessCtrl::EntityItem entity(tmpEncHash);

			std::vector<uint8_t> data;
			uint8_t retVal = ReadData(appKeyId, entity, data);

			if (retVal == EncFunc::FileOpRet::k_success)
			{
				RpcWriter rpcReturned(RpcWriter::CalcSizePrim<decltype(retVal)>() +
					RpcWriter::CalcSizeBin(data.size()), 2);
				auto rpcRetVal = rpcReturned.AddPrimitiveArg<decltype(retVal)>();
				auto rpcData = rpcReturned.AddBinaryArg(data.size());

				rpcRetVal = retVal;
				std::copy(data.begin(), data.end(), rpcData.begin());

				tls.SendRpc(rpcReturned);
			}
			else
			{
				RpcWriter rpcReturned(RpcWriter::CalcSizePrim<decltype(retVal)>() +
					RpcWriter::CalcSizeBin(0), 2);
				auto rpcRetVal = rpcReturned.AddPrimitiveArg<decltype(retVal)>();
				auto rpcData = rpcReturned.AddBinaryArg(0);

				rpcRetVal = retVal;

				tls.SendRpc(rpcReturned);
			}

			return false;
		}
		else
		{
			throw RuntimeException("Number of arguments for function k_findSuccessor doesn't match.");
		}

	case k_insertData:
		if (rpc.GetArgCount() == 4)
		{
			const auto& keyId = rpc.GetPrimitiveArg<uint8_t[DhtStates::sk_keySizeByte]>(); //Arg 2.
			const auto meta = rpc.GetBinaryArg(); //Arg 3.
			const auto data = rpc.GetBinaryArg(); //Arg 4.

			uint8_t appKeyId[DhtStates::sk_keySizeByte] = { 0 };
			Hasher::ArrayBatchedCalc<HashType::SHA256>(appKeyId, gsk_appKeyIdPrefix, keyId);

			uint8_t retVal = InsertData(appKeyId, meta.first, meta.second, data.first, data.second);

			RpcWriter rpcReturned(RpcWriter::CalcSizePrim<decltype(retVal)>(), 1);
			auto rpcRetVal = rpcReturned.AddPrimitiveArg<decltype(retVal)>();

			rpcRetVal = retVal;

			tls.SendRpc(rpcReturned);

			return false;
		}
		else
		{
			throw RuntimeException("Number of arguments for function k_findSuccessor doesn't match.");
		}

	case k_updateData:
		if (rpc.GetArgCount() == 3)
		{
			const auto& keyId = rpc.GetPrimitiveArg<uint8_t[DhtStates::sk_keySizeByte]>(); //Arg 2.
			const auto data = rpc.GetBinaryArg(); //Arg 3.

			uint8_t appKeyId[DhtStates::sk_keySizeByte] = { 0 };
			Hasher::ArrayBatchedCalc<HashType::SHA256>(appKeyId, gsk_appKeyIdPrefix, keyId);

			if (encHash.size() != sizeof(general_256bit_hash))
			{
				throw RuntimeException("Feature not implemented yet. (Enclave hash is not 256 bit)");
			}

			general_256bit_hash tmpEncHash;
			std::copy(encHash.begin(), encHash.end(), std::begin(tmpEncHash));

			AccessCtrl::EntityItem entity(tmpEncHash);

			uint8_t retVal = UpdateData(appKeyId, entity, data.first, data.second);

			RpcWriter rpcReturned(RpcWriter::CalcSizePrim<decltype(retVal)>(), 1);
			auto rpcRetVal = rpcReturned.AddPrimitiveArg<decltype(retVal)>();

			rpcRetVal = retVal;

			tls.SendRpc(rpcReturned);

			return false;
		}
		else
		{
			throw RuntimeException("Number of arguments for function k_findSuccessor doesn't match.");
		}

	case k_delData:
		if (rpc.GetArgCount() == 2)
		{
			const auto& keyId = rpc.GetPrimitiveArg<uint8_t[DhtStates::sk_keySizeByte]>(); //Arg 2.

			uint8_t appKeyId[DhtStates::sk_keySizeByte] = { 0 };
			Hasher::ArrayBatchedCalc<HashType::SHA256>(appKeyId, gsk_appKeyIdPrefix, keyId);

			if (encHash.size() != sizeof(general_256bit_hash))
			{
				throw RuntimeException("Feature not implemented yet. (Enclave hash is not 256 bit)");
			}

			general_256bit_hash tmpEncHash;
			std::copy(encHash.begin(), encHash.end(), std::begin(tmpEncHash));

			AccessCtrl::EntityItem entity(tmpEncHash);

			uint8_t retVal = DelData(appKeyId, entity);

			RpcWriter rpcReturned(RpcWriter::CalcSizePrim<decltype(retVal)>(), 1);
			auto rpcRetVal = rpcReturned.AddPrimitiveArg<decltype(retVal)>();

			rpcRetVal = retVal;

			tls.SendRpc(rpcReturned);

			return false;
		}
		else
		{
			throw RuntimeException("Number of arguments for function k_findSuccessor doesn't match.");
		}

	default: return false;
	}
}

bool Dht::AppFindSuccessor(Decent::Net::TlsCommLayer & tls, Net::EnclaveCntTranslator& cnt, const uint8_t(&keyId)[DhtStates::sk_keySizeByte])
{
	ConstBigNumber queriedId(keyId);

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

		std::shared_ptr<std::unique_ptr<EnclaveCntTranslator> > pendingCntPtr = 
			std::make_shared<std::unique_ptr<EnclaveCntTranslator> >(Tools::make_unique<EnclaveCntTranslator>(std::move(cnt)));
		std::shared_ptr<std::unique_ptr<TlsCommLayer> > pendingTlsPtr = 
			std::make_shared<std::unique_ptr<TlsCommLayer> >(Tools::make_unique<TlsCommLayer>(std::move(tls)));

		(*pendingTlsPtr)->SetConnectionPtr(*(*pendingCntPtr));

		std::function<void*(const ReplyQueueItem&)> callBackFunc = [pendingCntPtr, pendingTlsPtr](const ReplyQueueItem& item) -> void*
		{
			std::unique_ptr<EnclaveCntTranslator>& pendingCnt = *pendingCntPtr;
			std::unique_ptr<TlsCommLayer>& pendingTls = *pendingTlsPtr;

			if (pendingCnt && pendingTls)
			{
				pendingTls->SendStruct(item.m_resAddr);
				void* res = pendingCnt->GetPointer();
				pendingCnt.reset();
				pendingTls.reset();
				return res;
			}
			else
			{
				return nullptr;
			}
		};

		std::unique_ptr<AddrForwardQueueItem> queueItem = Tools::make_unique<AddrForwardQueueItem>();

		std::copy(std::begin(keyId), std::end(keyId), std::begin(queueItem->m_keyId));
		queueItem->m_reAddr = localNode->GetAddress();
		queueItem->m_reqId = reinterpret_cast<uint64_t>((*pendingTlsPtr).get());

		//PRINT_I("Forward query with ID %s to peer.", requestId.c_str());
		
		{
			std::unique_lock<std::mutex> clientPendingQueriesLock(gs_clientPendingQueriesMutex);
			gs_clientPendingQueries.insert(
				std::make_pair(queueItem->m_reqId, std::move(callBackFunc)));
		}

		DhtStates::DhtLocalNodeType::NodeBasePtr nextHop = localNode->GetNextHop(queriedId);

		{
			std::unique_lock<std::mutex> forwardQueueLock(gs_forwardQueueMutex);
			gs_addrForwardQueue.push(std::make_pair(nextHop->GetAddress(), std::move(queueItem)));
			forwardQueueLock.unlock();
			gs_forwardQueueSignal.notify_one();

			//NOTE! Don't use 'ConstBigNumber queriedId' after here!
		}

		return true;
	}
}
