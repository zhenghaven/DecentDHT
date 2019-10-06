#include "DhtServer.h"

#include <queue>
#include <functional>

#include <cppcodec/base64_default_rfc4648.hpp>

#include <DecentApi/Common/Common.h>
#include <DecentApi/Common/make_unique.h>
#include <DecentApi/Common/GeneralKeyTypes.h>

#include <DecentApi/Common/MbedTls/Hasher.h>
#include <DecentApi/Common/MbedTls/X509Cert.h>
#include <DecentApi/Common/MbedTls/BigNumber.h>

#include <DecentApi/Common/Net/TlsCommLayer.h>
#include <DecentApi/Common/Net/ConnectionBase.h>
#include <DecentApi/Common/Net/RpcParser.h>
#include <DecentApi/Common/Net/RpcWriter.h>

#include <DecentApi/Common/Ra/Crypto.h>
#include <DecentApi/Common/Ra/KeyContainer.h>

#include <DecentApi/CommonEnclave/Net/EnclaveCntTranslator.h>
#include <DecentApi/CommonEnclave/Ra/TlsConfigSameEnclave.h>

#if !defined(ACCESS_CONTROL_NO_CACHE) && !defined(ENCLAVE_PLATFORM_NON_ENCLAVE)
#include <DecentApi/CommonEnclave/Tools/DataSealer.h>
#endif

#include "../../Common/Dht/LocalNode.h"
#include "../../Common/Dht/FuncNums.h"
#include "../../Common/Dht/NodeBase.h"

#include "../../Common/Dht/AccessCtrl/FullPolicy.h"
#include "../../Common/Dht/AccessCtrl/EntityList.h"
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
		node->GetNodeId().ToBinary(keyBin.Get());

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

		try
		{
			std::vector<uint8_t> meta;
			std::vector<uint8_t> data = gs_state.GetDhtStore().GetValue(queriedId, meta);

			auto it = data.cbegin();
			AccessCtrl::EntityList attrListObj(it, data.cend());

			AccessCtrl::EntityItem testEntity(userId);

			return attrListObj.Search(testEntity) ? 1 : 0;
		}
		catch (const std::exception&)
		{
			return 0; //false;
		}
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
		static std::shared_ptr<Ra::TlsConfigSameEnclave> tlsCfg = std::make_shared<Ra::TlsConfigSameEnclave>(gs_state, TlsConfig::Mode::ClientHasCert, nullptr);
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

		tls.SendRawAll(keyBin.data(), keyBin.size()); //2. Send start key.
		end.ToBinary(keyBin);
		tls.SendRawAll(keyBin.data(), keyBin.size()); //3. Send end key.

		dhtStore.RecvMigratingData(
			[&tls](void* buffer, const size_t size) -> void
		{
			tls.RecvRawAll(buffer, size);
		},
			[&tls]() -> BigNumber
		{
			std::array<uint8_t, DhtStates::sk_keySizeByte> keyBuf{};
			tls.RecvRawAll(keyBuf.data(), keyBuf.size());
			return BigNumber(keyBuf, true);
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
			tls.SendRawAll(buffer, size);
		},
			[&tls](const BigNumber& key) -> void
		{
			std::array<uint8_t, DhtStates::sk_keySizeByte> keyBuf{};
			key.ToBinary(keyBuf);
			tls.SendRawAll(keyBuf.data(), keyBuf.size());
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

	BigNumber step = largest / static_cast<int64_t>(totalNode);

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
	//std::queue<std::pair<uint64_t, std::unique_ptr<AddrForwardQueueItem> > > tmpAddrQueue;
	//std::queue<std::pair<uint64_t, std::unique_ptr<AttrListForwardQueueItem> > > tmpListQueue;

	uint64_t addr = 0;
	std::unique_ptr<AddrForwardQueueItem> addrItem;
	std::unique_ptr<AttrListForwardQueueItem> attrItem;

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
				//tmpAddrQueue.swap(gs_addrForwardQueue);
				addr = gs_addrForwardQueue.front().first;
				addrItem = std::move(gs_addrForwardQueue.front().second);
				gs_addrForwardQueue.pop();
			}
			else if (!gs_isForwardingTerminated && gs_attrListForwardQueue.size() > 0)
			//if (!gs_isForwardingTerminated && gs_attrListForwardQueue.size() > 0)
			{
				//tmpListQueue.swap(gs_attrListForwardQueue);
				addr = gs_attrListForwardQueue.front().first;
				attrItem = std::move(gs_attrListForwardQueue.front().second);
				gs_attrListForwardQueue.pop();
			}
		}

		if(addrItem)
		//while (tmpAddrQueue.size() > 0)
		{
			//ForwardAddrQuery(tmpAddrQueue.front().first, *tmpAddrQueue.front().second);
			//tmpAddrQueue.pop();
			ForwardAddrQuery(addr, *addrItem);
		}
		else if(attrItem)
		//while (tmpListQueue.size() > 0)
		{
			//ForwardListQuery(tmpListQueue.front().first, *tmpListQueue.front().second);
			//tmpListQueue.pop();
			ForwardListQuery(addr, *attrItem);
		}
	}
}

void Dht::QueryReplyWorker()
{
	//std::queue<std::pair< uint64_t, std::unique_ptr<ReplyQueueItem> > > tmpQueue;
	//bool hasJob = false;
	uint64_t addr = 0;
	std::unique_ptr<ReplyQueueItem> item;

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
				//tmpQueue.swap(gs_replyQueue);
				//hasJob = true;
				addr = gs_replyQueue.front().first;
				item = std::move(gs_replyQueue.front().second);
				gs_replyQueue.pop();
			}
		}

		if (item)
		//while (tmpQueue.size() > 0)
		{
			//ReplyQuery(tmpQueue.front().first, *tmpQueue.front().second);
			//tmpQueue.pop();
			ReplyQuery(addr, *item);
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

void Dht::GetNodeId(Decent::Net::SecureCommLayer & secComm)
{
    //LOGI("DHT Server: Getting the NodeId...");
	DhtStates::DhtLocalNodePtrType localNode = gs_state.GetDhtNode();

	constexpr size_t rpcSize = RpcWriter::CalcSizePrim<uint8_t[DhtStates::sk_keySizeByte]>();

	RpcWriter rpc(rpcSize, 1);

	auto keyBin = rpc.AddPrimitiveArg<uint8_t[DhtStates::sk_keySizeByte]>();
	localNode->GetNodeId().ToBinary(keyBin.Get());

	secComm.SendRpc(rpc);
}

void Dht::FindSuccessor(Decent::Net::SecureCommLayer & secComm, const uint8_t(&keyId)[DhtStates::sk_keySizeByte])
{
	ConstBigNumber queriedId(keyId);
	//LOGI("Recv queried ID: %s.", static_cast<const BigNumber&>(queriedId).ToBigEndianHexStr().c_str());

	DhtStates::DhtLocalNodePtrType localNode = gs_state.GetDhtNode();

	ReturnNode(secComm, localNode->FindSuccessor(queriedId));
}

void Dht::FindPredecessor(Decent::Net::SecureCommLayer & secComm, const uint8_t(&keyId)[DhtStates::sk_keySizeByte])
{
	ConstBigNumber queriedId(keyId);
    //LOGI("Recv queried ID: %s.", static_cast<const BigNumber&>(queriedId).ToBigEndianHexStr().c_str());

	DhtStates::DhtLocalNodePtrType localNode = gs_state.GetDhtNode();

	ReturnNode(secComm, localNode->FindPredecessor(queriedId));
}

void Dht::GetImmediateSucessor(Decent::Net::SecureCommLayer & secComm)
{
    //LOGI("DHT Server: Finding Immediate Successor...");

    DhtStates::DhtLocalNodePtrType localNode = gs_state.GetDhtNode();

	ReturnNode(secComm, localNode->GetImmediateSuccessor());
}

void Dht::GetImmediatePredecessor(Decent::Net::SecureCommLayer & secComm)
{
	//LOGI("DHT Server: Getting Immediate Predecessor...");

	DhtStates::DhtLocalNodePtrType localNode = gs_state.GetDhtNode();

	ReturnNode(secComm, localNode->GetImmediatePredecessor());
}

void Dht::SetImmediatePredecessor(Decent::Net::SecureCommLayer & secComm, const uint8_t(&keyId)[DhtStates::sk_keySizeByte], uint64_t addr)
{
	//LOGI("DHT Server: Setting Immediate Predecessor...");

	DhtStates::DhtLocalNodePtrType localNode = gs_state.GetDhtNode();

	localNode->SetImmediatePredecessor(std::make_shared<NodeConnector>(addr, BigNumber(keyId, true))); //2. Receive Node. - Done!

	secComm.SendRpc(RpcWriter(0, 0));
}

void Dht::UpdateFingerTable(Decent::Net::SecureCommLayer & secComm, const uint8_t(&keyId)[DhtStates::sk_keySizeByte], uint64_t addr, uint64_t i)
{
	PRINT_I("Updating finger table; i = %llu.", i);

	DhtStates::DhtLocalNodeType::NodeBasePtr s = std::make_shared<NodeConnector>(addr, BigNumber(keyId, true));

	DhtStates::DhtLocalNodePtrType localNode = gs_state.GetDhtNode();

	localNode->UpdateFingerTable(s, i);

	secComm.SendRpc(RpcWriter(0, 0));
}

void Dht::DeUpdateFingerTable(Decent::Net::SecureCommLayer & secComm, const uint8_t(&oldId)[DhtStates::sk_keySizeByte], const uint8_t(&keyId)[DhtStates::sk_keySizeByte], uint64_t addr, uint64_t i)
{
	PRINT_I("De-updating finger table; i = %llu.", i);

	DhtStates::DhtLocalNodeType::NodeBasePtr s = std::make_shared<NodeConnector>(addr, BigNumber(keyId, true));

	DhtStates::DhtLocalNodePtrType localNode = gs_state.GetDhtNode();

	localNode->DeUpdateFingerTable(ConstBigNumber(oldId), s, i);

	secComm.SendRpc(RpcWriter(0, 0));
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
		LOGI("Forwarding list attribute query...");
		std::unique_lock<std::mutex> forwardQueueLock(gs_forwardQueueMutex);
		gs_attrListForwardQueue.push(std::make_pair(nextHop->GetAddress(), std::move(forwardItem)));
		forwardQueueLock.unlock();
		gs_forwardQueueSignal.notify_one();
	}
}

void Dht::ProcessDhtQuery(Decent::Net::SecureCommLayer & secComm, void*& heldCntPtr)
{
	using namespace EncFunc::Dht;

	//LOGI("DHT Server: Processing DHT queries...");

	RpcParser rpc(secComm.RecvContainer<std::vector<uint8_t> >());
	const NumType& funcNum = rpc.GetPrimitiveArg<NumType>();

	switch (funcNum)
	{

	case k_getNodeId:
		if (rpc.GetArgCount() == 1)
		{
			GetNodeId(secComm);
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
			FindSuccessor(secComm, keyId);
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
			FindPredecessor(secComm, keyId);
		}
		else
		{
			throw RuntimeException("Number of arguments for function k_findPredecessor doesn't match.");
		}
		break;

	case k_getImmediateSuc:
		if (rpc.GetArgCount() == 1)
		{
			GetImmediateSucessor(secComm);
		}
		else
		{
			throw RuntimeException("Number of arguments for function k_getImmediateSuc doesn't match.");
		}
		break;

	case k_getImmediatePre:
		if (rpc.GetArgCount() == 1)
		{
			GetImmediatePredecessor(secComm);
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
			SetImmediatePredecessor(secComm, keyId, addr);
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
			UpdateFingerTable(secComm, keyId, addr, i);
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
			DeUpdateFingerTable(secComm, oldId, keyId, addr, i);
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

void Dht::ProcessStoreRequest(Decent::Net::SecureCommLayer & secComm)
{
	using namespace EncFunc::Store;

	NumType funcNum;
	secComm.RecvStruct(funcNum); //1. Received function type.

	switch (funcNum)
	{
	case k_getMigrateData:
		GetMigrateData(secComm);
		break;

	case k_setMigrateData:
		SetMigrateData(secComm);
		break;

	default:
		break;
	}
}

void Dht::GetMigrateData(Decent::Net::SecureCommLayer & secComm)
{
	std::array<uint8_t, DhtStates::sk_keySizeByte> startKeyBin{};
	secComm.RecvRawAll(startKeyBin.data(), startKeyBin.size());
	ConstBigNumber start(startKeyBin);

	std::array<uint8_t, DhtStates::sk_keySizeByte> endKeyBin{};
	secComm.RecvRawAll(endKeyBin.data(), endKeyBin.size());
	ConstBigNumber end(endKeyBin);

	gs_state.GetDhtStore().SendMigratingData(
		[&secComm](const void* buffer, const size_t size) -> void
	{
		secComm.SendRawAll(buffer, size);
	},
		[&secComm](const BigNumber& key) -> void
	{
		std::array<uint8_t, DhtStates::sk_keySizeByte> keyBuf{};
		key.ToBinary(keyBuf);
		secComm.SendRawAll(keyBuf.data(), keyBuf.size());
	},
		start, end);
}

void Dht::SetMigrateData(Decent::Net::SecureCommLayer & secComm)
{
	gs_state.GetDhtStore().RecvMigratingData(
		[&secComm](void* buffer, const size_t size) -> void
	{
		secComm.RecvRawAll(buffer, size);
	},
		[&secComm]() -> BigNumber
	{
		std::array<uint8_t, DhtStates::sk_keySizeByte> keyBuf{};
		secComm.RecvRawAll(keyBuf.data(), keyBuf.size());
		return BigNumber(keyBuf, true);
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

bool Dht::ProcessAppRequest(std::unique_ptr<SecureCommLayer> & secCnt, Net::EnclaveCntTranslator& cnt, const std::vector<uint8_t>& encHash)
{
	if (!secCnt)
	{
		throw RuntimeException("Pointer to secure comm layer is null.");
	}

	using namespace EncFunc::App;

	RpcParser rpc(secCnt->RecvContainer<std::vector<uint8_t> >());

	const auto& funcNum = rpc.GetPrimitiveArg<NumType>(); //Arg 1.

	switch (funcNum)
	{
	case k_findSuccessor:
		if (rpc.GetArgCount() == 2)
		{
			const auto& keyId = rpc.GetPrimitiveArg<uint8_t[DhtStates::sk_keySizeByte]>(); //Arg 2.

			uint8_t appKeyId[DhtStates::sk_keySizeByte] = { 0 };
			Hasher<HashType::SHA256>().Calc(appKeyId, gsk_appKeyIdPrefix, keyId);

			return AppFindSuccessor(secCnt, cnt, appKeyId);
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
			Hasher<HashType::SHA256>().Calc(appKeyId, gsk_appKeyIdPrefix, keyId);

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

				secCnt->SendRpc(rpcReturned);
			}
			else
			{
				RpcWriter rpcReturned(RpcWriter::CalcSizePrim<decltype(retVal)>() +
					RpcWriter::CalcSizeBin(0), 2);
				auto rpcRetVal = rpcReturned.AddPrimitiveArg<decltype(retVal)>();
				auto rpcData = rpcReturned.AddBinaryArg(0);

				rpcRetVal = retVal;

				secCnt->SendRpc(rpcReturned);
			}

			return false;
		}
		else
		{
			throw RuntimeException("Number of arguments for function k_readData doesn't match.");
		}

	case k_insertData:
		if (rpc.GetArgCount() == 4)
		{
			const auto& keyId = rpc.GetPrimitiveArg<uint8_t[DhtStates::sk_keySizeByte]>(); //Arg 2.
			auto meta = rpc.GetBinaryArg(); //Arg 3.
			auto data = rpc.GetBinaryArg(); //Arg 4.

			FullPolicy verifiedPolicy(meta.first, meta.second);
			std::vector<uint8_t> verifiedPolicyBin(verifiedPolicy.GetSerializedSize());
			verifiedPolicy.Serialize(verifiedPolicyBin.begin(), verifiedPolicyBin.end());

			uint8_t appKeyId[DhtStates::sk_keySizeByte] = { 0 };
			Hasher<HashType::SHA256>().Calc(appKeyId, gsk_appKeyIdPrefix, keyId);

			uint8_t retVal = InsertData(appKeyId, verifiedPolicyBin.begin(), verifiedPolicyBin.end(), data.first, data.second);

			RpcWriter rpcReturned(RpcWriter::CalcSizePrim<decltype(retVal)>(), 1);
			auto rpcRetVal = rpcReturned.AddPrimitiveArg<decltype(retVal)>();

			rpcRetVal = retVal;

			secCnt->SendRpc(rpcReturned);

			return false;
		}
		else
		{
			throw RuntimeException("Number of arguments for function k_insertData doesn't match.");
		}

	case k_updateData:
		if (rpc.GetArgCount() == 3)
		{
			const auto& keyId = rpc.GetPrimitiveArg<uint8_t[DhtStates::sk_keySizeByte]>(); //Arg 2.
			const auto data = rpc.GetBinaryArg(); //Arg 3.

			uint8_t appKeyId[DhtStates::sk_keySizeByte] = { 0 };
			Hasher<HashType::SHA256>().Calc(appKeyId, gsk_appKeyIdPrefix, keyId);

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

			secCnt->SendRpc(rpcReturned);

			return false;
		}
		else
		{
			throw RuntimeException("Number of arguments for function k_updateData doesn't match.");
		}

	case k_delData:
		if (rpc.GetArgCount() == 2)
		{
			const auto& keyId = rpc.GetPrimitiveArg<uint8_t[DhtStates::sk_keySizeByte]>(); //Arg 2.

			uint8_t appKeyId[DhtStates::sk_keySizeByte] = { 0 };
			Hasher<HashType::SHA256>().Calc(appKeyId, gsk_appKeyIdPrefix, keyId);

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

			secCnt->SendRpc(rpcReturned);

			return false;
		}
		else
		{
			throw RuntimeException("Number of arguments for function k_delData doesn't match.");
		}

	default: throw RuntimeException("Unknown function is called by the client application.");
	}
}

namespace
{
	static bool AppFindSuccessorForwardProc(std::unique_ptr<SecureCommLayer> secCnt, Net::EnclaveCntTranslator& cnt,
		const uint8_t(&keyId)[DhtStates::sk_keySizeByte], const MbedTlsObj::BigNumber& queriedId,
		DhtStates::DhtLocalNodePtrType localNode)
	{
		std::shared_ptr<std::unique_ptr<EnclaveCntTranslator> > pendingCntPtr =
			std::make_shared<std::unique_ptr<EnclaveCntTranslator> >(Tools::make_unique<EnclaveCntTranslator>(std::move(cnt)));
		std::shared_ptr<std::unique_ptr<SecureCommLayer> > pendingTlsPtr =
			std::make_shared<std::unique_ptr<SecureCommLayer> >(std::move(secCnt));

		(*pendingTlsPtr)->SetConnectionPtr(*(*pendingCntPtr));

		std::function<void*(const ReplyQueueItem&)> callBackFunc = [pendingCntPtr, pendingTlsPtr](const ReplyQueueItem& item) -> void*
		{
			void* res = nullptr;

			if (pendingCntPtr && pendingTlsPtr && *pendingCntPtr && *pendingTlsPtr)
			{
				(*pendingTlsPtr)->SendStruct(item.m_resAddr);

				res = (*pendingCntPtr)->GetPointer();

				(*pendingTlsPtr).reset();
				(*pendingCntPtr).reset();
			}

			return res;
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

bool Dht::AppFindSuccessor(std::unique_ptr<SecureCommLayer> & secCnt, Net::EnclaveCntTranslator& cnt, const uint8_t(&keyId)[DhtStates::sk_keySizeByte])
{
	ConstBigNumber queriedId(keyId);

	//LOGI("Recv app queried ID: %s.", static_cast<const BigNumber&>(queriedId).ToBigEndianHexStr().c_str());

	DhtStates::DhtLocalNodePtrType localNode = gs_state.GetDhtNode();
	
	uint64_t resAddr = 0;
	if (TryGetQueriedAddrLocally(*localNode, queriedId, resAddr))
	{
		//Query can be answered immediately.

		secCnt->SendStruct(resAddr);

		return false;
	}
	else
	{
		//Query should be forwarded to peer(s) and reply later.

		return AppFindSuccessorForwardProc(std::move(secCnt), cnt, keyId, queriedId, localNode);
	}
}

namespace
{
	struct HashArrayWarp
	{
		general_256bit_hash m_h;
	};

	static HashArrayWarp ConstructSelfHashStruct(const std::vector<uint8_t>& selfHash)
	{
		if (selfHash.size() != sizeof(general_256bit_hash))
		{
			throw Decent::RuntimeException("Feature not implemented yet. (Enclave hash is not 256 bit)");
		}

		HashArrayWarp res;

		std::copy(selfHash.begin(), selfHash.end(), std::begin(res.m_h));

		return res;
	}

	constexpr const char gsk_attrListSealKeyLabel[] = "AttrListSealKey";
}

static bool VerifyAtListCacheSignature(const std::vector<uint8_t>& sealedList, std::vector<uint8_t>& unsealedList)
{
#if !defined(ACCESS_CONTROL_NO_CACHE) && !defined(ENCLAVE_PLATFORM_NON_ENCLAVE)
	try
	{
		std::vector<uint8_t> meta;
		Tools::DataSealer::UnsealData(Tools::DataSealer::KeyPolicy::ByMrEnclave, gs_state, gsk_attrListSealKeyLabel, sealedList, meta, unsealedList, nullptr);

		return true;
	}
	catch (const std::exception&)
	{
		return false;
	}
#else
	return false;
#endif
}

static std::unique_ptr<AccessCtrl::AbAttributeList> ParseCachedAttList(uint8_t hasCache, std::pair<std::vector<uint8_t>::const_iterator, std::vector<uint8_t>::const_iterator> cacheBin)
{
	if (hasCache)
	{
		std::vector<uint8_t> unsealedList;
		if (VerifyAtListCacheSignature(std::vector<uint8_t>(cacheBin.first, cacheBin.second), unsealedList))
		{
			auto it = unsealedList.cbegin();
			return Tools::make_unique<AccessCtrl::AbAttributeList>(it, unsealedList.cend());
		}
	}
	
	return Tools::make_unique<AccessCtrl::AbAttributeList>();
}

static void LocalAttrListCollecter(const general_256bit_hash& reqUserId, const std::set<AccessCtrl::AbAttributeItem>& neededList,
	std::shared_ptr<std::set<AccessCtrl::AbAttributeItem> > approvedList,
	std::map<AccessCtrl::AbAttributeItem, std::pair<uint64_t, std::array<uint8_t, DhtStates::sk_keySizeByte> > >& remoteItems)
{
	DhtStates::DhtLocalNodePtrType localNode = gs_state.GetDhtNode();

	for (const auto& neededItem : neededList)
	{
		std::array<uint8_t, DhtStates::sk_keySizeByte> attrListId;
		Hasher<HashType::SHA256>().Calc(attrListId, gsk_atListKeyIdPrefix, neededItem.GetUserHash(), neededItem.GetAttrHash());
		ConstBigNumber attrListIdNum(attrListId);

		auto nextHop = GetAttrListQueryNextHop(*localNode, attrListIdNum);

		if (!nextHop)
		{
			//Local query
			
			if (CheckUserIdInAttrList(attrListIdNum, reqUserId))
			{
				approvedList->insert(neededItem);
			}
		}
		else
		{
			//Remote query
			
			remoteItems.insert(std::make_pair(neededItem, std::make_pair(nextHop->GetAddress(), attrListId)));
		}
	}
}

static void RemoteAttrListCollecter(const general_256bit_hash& reqUserId,
	const std::map<AccessCtrl::AbAttributeItem, std::pair<uint64_t, std::array<uint8_t, DhtStates::sk_keySizeByte> > >& remoteItems,
	std::shared_ptr<std::set<AccessCtrl::AbAttributeItem> > existList, std::shared_ptr<std::set<AccessCtrl::AbAttributeItem> > approvedList,
	std::function<void*(const AccessCtrl::AbAttributeList&, const AccessCtrl::AbAttributeList&)> callBackAfterCollect)
{
	DhtStates::DhtLocalNodePtrType localNode = gs_state.GetDhtNode();

	std::shared_ptr<std::mutex> apprListMutex = std::make_shared<std::mutex>();
	std::shared_ptr<size_t> pendingCount = std::make_shared<size_t>(remoteItems.size());

	for (const auto& item : remoteItems)
	{
		std::shared_ptr<AccessCtrl::AbAttributeItem> attrListItem = std::make_shared<AccessCtrl::AbAttributeItem>(item.first);

		//Construct forward queue item
		std::unique_ptr<AttrListForwardQueueItem> forwardQueueItem = Tools::make_unique<AttrListForwardQueueItem>();

		forwardQueueItem->m_reqId = reinterpret_cast<uint64_t>(attrListItem.get());
		forwardQueueItem->m_reAddr = localNode->GetAddress();
		std::copy(item.second.second.begin(), item.second.second.end(), std::begin(forwardQueueItem->m_keyId));
		std::copy(std::begin(reqUserId), std::end(reqUserId), std::begin(forwardQueueItem->m_userId));

		//Reply queue callback
		auto replyQueueCallBack = [apprListMutex, pendingCount, attrListItem, existList, approvedList, callBackAfterCollect](const ReplyQueueItem& rpQueueItem) -> void*
		{
			std::unique_lock<std::mutex> apprListLock(*apprListMutex);

			if (*pendingCount == 0) //Just to be safe.
			{
				return nullptr;
			}

			if (rpQueueItem.m_resAddr)
			{
				approvedList->insert(*attrListItem);
			}

			(*pendingCount)--;

			if (*pendingCount == 0)
			{
				return callBackAfterCollect(AccessCtrl::AbAttributeList(std::move(*existList)),
					AccessCtrl::AbAttributeList(std::move(*approvedList)));
			}
			else
			{
				return nullptr;
			}
		};

		//Insert to pending queue first
		{
			std::unique_lock<std::mutex> clientPendingQueriesLock(gs_clientPendingQueriesMutex);
			gs_clientPendingQueries.insert(std::make_pair(forwardQueueItem->m_reqId, replyQueueCallBack));
		}

		//Insert to forwardqueue
		{
			LOGI("Forwarding list attribute query...");
			std::unique_lock<std::mutex> forwardQueueLock(gs_forwardQueueMutex);
			gs_attrListForwardQueue.push(std::make_pair(item.second.first, std::move(forwardQueueItem)));
			forwardQueueLock.unlock();
			gs_forwardQueueSignal.notify_one();
		}
	}
}

//#define ACCESS_CONTROL_NO_CACHE

static void UserReadReturnWithCache(Decent::Net::TlsCommLayer & tls, const uint8_t (&dataId)[DhtStates::sk_keySizeByte], const AccessCtrl::AbAttributeList& listForCache)
{
	std::string certPem = gs_state.GetCertContainer().GetCert()->GetPemChain();

	AccessCtrl::AbAttributeList tmpList;
	std::vector<uint8_t> data;
	uint8_t retVal = ReadData(dataId, listForCache, tmpList, data);

	std::vector<uint8_t> sealedList;

#if !defined(ACCESS_CONTROL_NO_CACHE) && !defined(ENCLAVE_PLATFORM_NON_ENCLAVE)
	std::vector<uint8_t> unsealedList;
	unsealedList.resize(listForCache.GetSerializedSize());
	listForCache.Serialize(unsealedList.begin());

	sealedList = Tools::DataSealer::SealData(Tools::DataSealer::KeyPolicy::ByMrEnclave, gs_state, gsk_attrListSealKeyLabel, std::array<uint8_t, 0>(), unsealedList);
#endif

	RpcWriter rpcReturned(RpcWriter::CalcSizePrim<decltype(retVal)>() +
		RpcWriter::CalcSizeBin(data.size()) +
		RpcWriter::CalcSizePrim<uint8_t>() +
		RpcWriter::CalcSizeBin(sealedList.size()), 4);

	auto rpcRetVal = rpcReturned.AddPrimitiveArg<decltype(retVal)>();
	auto rpcRetData = rpcReturned.AddBinaryArg(data.size());
	auto rpcRetHasCache = rpcReturned.AddPrimitiveArg<uint8_t>();
	auto rpcRetListBin = rpcReturned.AddBinaryArg(sealedList.size());

	rpcRetVal = retVal;

	if (rpcRetVal == EncFunc::FileOpRet::k_success)
	{
		std::copy(data.begin(), data.end(), rpcRetData.begin());
	}

#if !defined(ACCESS_CONTROL_NO_CACHE) && !defined(ENCLAVE_PLATFORM_NON_ENCLAVE)
	rpcRetHasCache = 1;

	std::copy(sealedList.begin(), sealedList.end(), rpcRetListBin.begin());
#else
	rpcRetHasCache = 0;
#endif

	tls.SendRpc(rpcReturned);
}

static void UserReadReturnWithoutCache(Decent::Net::TlsCommLayer & tls, uint8_t retVal, const std::vector<uint8_t>& data)
{
	RpcWriter rpcReturned(RpcWriter::CalcSizePrim<decltype(retVal)>() +
		RpcWriter::CalcSizeBin(data.size()) +
		RpcWriter::CalcSizePrim<uint8_t>(), 3);

	auto rpcRetVal = rpcReturned.AddPrimitiveArg<decltype(retVal)>();
	auto rpcRetData = rpcReturned.AddBinaryArg(data.size());
	auto rpcRetHasCache = rpcReturned.AddPrimitiveArg<uint8_t>();

	rpcRetVal = retVal;

	if (rpcRetVal == EncFunc::FileOpRet::k_success)
	{
		std::copy(data.begin(), data.end(), rpcRetData.begin());
	}
	rpcRetHasCache = 0;

	tls.SendRpc(rpcReturned);
}

static void UserUpdateReturnWithCache(Decent::Net::TlsCommLayer & tls, const uint8_t(&dataId)[DhtStates::sk_keySizeByte], const AccessCtrl::AbAttributeList& listForCache, const std::vector<uint8_t>& data)
{
	std::string certPem = gs_state.GetCertContainer().GetCert()->GetPemChain();

	AccessCtrl::AbAttributeList tmpList;
	uint8_t retVal = UpdateData(dataId, listForCache, tmpList, data.begin(), data.end());

	std::vector<uint8_t> sealedList;

#if !defined(ACCESS_CONTROL_NO_CACHE) && !defined(ENCLAVE_PLATFORM_NON_ENCLAVE)
	std::vector<uint8_t> unsealedList;
	unsealedList.resize(listForCache.GetSerializedSize());
	listForCache.Serialize(unsealedList.begin());

	sealedList = Tools::DataSealer::SealData(Tools::DataSealer::KeyPolicy::ByMrEnclave, gs_state, gsk_attrListSealKeyLabel, std::array<uint8_t, 0>(), unsealedList);
#endif

	RpcWriter rpcReturned(RpcWriter::CalcSizePrim<decltype(retVal)>() +
		RpcWriter::CalcSizePrim<uint8_t>() +
		RpcWriter::CalcSizeBin(sealedList.size()), 3);

	auto rpcRetVal = rpcReturned.AddPrimitiveArg<decltype(retVal)>();
	auto rpcRetHasCache = rpcReturned.AddPrimitiveArg<uint8_t>();
	auto rpcRetListBin = rpcReturned.AddBinaryArg(sealedList.size());

	rpcRetVal = retVal;

#if !defined(ACCESS_CONTROL_NO_CACHE) && !defined(ENCLAVE_PLATFORM_NON_ENCLAVE)
	rpcRetHasCache = 1;

	std::copy(sealedList.begin(), sealedList.end(), rpcRetListBin.begin());
#else
	rpcRetHasCache = 0;
#endif

	tls.SendRpc(rpcReturned);
}

static void UserUpdateReturnWithoutCache(Decent::Net::TlsCommLayer & tls, uint8_t retVal)
{
	RpcWriter rpcReturned(RpcWriter::CalcSizePrim<decltype(retVal)>() +
		RpcWriter::CalcSizePrim<uint8_t>(), 2);

	auto rpcRetVal = rpcReturned.AddPrimitiveArg<decltype(retVal)>();
	auto rpcRetHasCache = rpcReturned.AddPrimitiveArg<uint8_t>();

	rpcRetVal = retVal;
	rpcRetHasCache = 0;

	tls.SendRpc(rpcReturned);
}

static void UserDeleteReturnWithCache(Decent::Net::TlsCommLayer & tls, const uint8_t(&dataId)[DhtStates::sk_keySizeByte], const AccessCtrl::AbAttributeList& listForCache)
{
	std::string certPem = gs_state.GetCertContainer().GetCert()->GetPemChain();

	AccessCtrl::AbAttributeList tmpList;
	uint8_t retVal = DelData(dataId, listForCache, tmpList);

	std::vector<uint8_t> sealedList;

#if !defined(ACCESS_CONTROL_NO_CACHE) && !defined(ENCLAVE_PLATFORM_NON_ENCLAVE)
	std::vector<uint8_t> unsealedList;
	unsealedList.resize(listForCache.GetSerializedSize());
	listForCache.Serialize(unsealedList.begin());

	sealedList = Tools::DataSealer::SealData(Tools::DataSealer::KeyPolicy::ByMrEnclave, gs_state, gsk_attrListSealKeyLabel, std::array<uint8_t, 0>(), unsealedList);
#endif

	RpcWriter rpcReturned(RpcWriter::CalcSizePrim<decltype(retVal)>() +
		RpcWriter::CalcSizePrim<uint8_t>() +
		RpcWriter::CalcSizeBin(sealedList.size()), 5);

	auto rpcRetVal = rpcReturned.AddPrimitiveArg<decltype(retVal)>();
	auto rpcRetHasCache = rpcReturned.AddPrimitiveArg<uint8_t>();
	auto rpcRetListBin = rpcReturned.AddBinaryArg(sealedList.size());

	rpcRetVal = retVal;

#if !defined(ACCESS_CONTROL_NO_CACHE) && !defined(ENCLAVE_PLATFORM_NON_ENCLAVE)
	rpcRetHasCache = 1;

	std::copy(sealedList.begin(), sealedList.end(), rpcRetListBin.begin());
#else
	rpcRetHasCache = 0;
#endif

	tls.SendRpc(rpcReturned);
}

static void UserDeleteReturnWithoutCache(Decent::Net::TlsCommLayer & tls, uint8_t retVal)
{
	RpcWriter rpcReturned(RpcWriter::CalcSizePrim<decltype(retVal)>() +
		RpcWriter::CalcSizePrim<uint8_t>(), 2);

	auto rpcRetVal = rpcReturned.AddPrimitiveArg<decltype(retVal)>();
	auto rpcRetHasCache = rpcReturned.AddPrimitiveArg<uint8_t>();

	rpcRetVal = retVal;
	rpcRetHasCache = 0;

	tls.SendRpc(rpcReturned);
}

bool Dht::ProcessUserRequest(std::unique_ptr<TlsCommLayer> & tls, Net::EnclaveCntTranslator & cnt, const std::vector<uint8_t>& selfHash)
{
	static const HashArrayWarp selfHashArray = ConstructSelfHashStruct(selfHash);

	if (!tls)
	{
		throw RuntimeException("Pointer to secure comm layer is null.");
	}

	using namespace EncFunc::User;

	RpcParser rpc(tls->RecvContainer<std::vector<uint8_t> >());

	const auto& funcNum = rpc.GetPrimitiveArg<NumType>(); //Arg 1.

	switch (funcNum)
	{
	case k_findSuccessor:
		if (rpc.GetArgCount() == 2)
		{
			const auto& keyId = rpc.GetPrimitiveArg<uint8_t[DhtStates::sk_keySizeByte]>(); //Arg 2

			uint8_t appKeyId[DhtStates::sk_keySizeByte] = { 0 };
			Hasher<HashType::SHA256>().Calc(appKeyId, gsk_appKeyIdPrefix, keyId);

			return AppFindSuccessor(tls, cnt, appKeyId);
		}
		else
		{
			throw RuntimeException("Number of arguments for function k_findSuccessor doesn't match.");
		}

	case k_readData:
		if (rpc.GetArgCount() == 4)
		{
			const auto& keyId = rpc.GetPrimitiveArg<uint8_t[DhtStates::sk_keySizeByte]>(); //Arg 2
			const auto& validCache = rpc.GetPrimitiveArg<uint8_t>(); //Arg 3
			auto cacheBin = rpc.GetBinaryArg(); //Arg 4

			//User ID:
			std::string userKeyPem = tls->GetPublicKeyPem();
			general_256bit_hash userId = { 0 };
			Hasher<HashType::SHA256>().Calc(userId, userKeyPem);

			std::unique_ptr<AccessCtrl::AbAttributeList> exListCachePtr = ParseCachedAttList(validCache, cacheBin);
			std::unique_ptr<AccessCtrl::AbAttributeList> neededAttrListPtr = Tools::make_unique<AccessCtrl::AbAttributeList>();

			uint8_t dataKeyId[DhtStates::sk_keySizeByte] = { 0 };
			Hasher<HashType::SHA256>().Calc(dataKeyId, gsk_appKeyIdPrefix, keyId);

			std::vector<uint8_t> data;
			uint8_t retVal = ReadData(dataKeyId, *exListCachePtr, *neededAttrListPtr, data);

			if (retVal != EncFunc::FileOpRet::k_denied || neededAttrListPtr->GetSize() == 0)
			{ //Can return immediately
				UserReadReturnWithoutCache(*tls, retVal, data);

				return false;
			}

			//Start from here, retVal == denied && neededAttrList.size() > 0
			
			//Local Attr list collector
			std::shared_ptr<std::set<AccessCtrl::AbAttributeItem> > approvedList = std::make_shared<std::set<AccessCtrl::AbAttributeItem> >();
			std::map<AccessCtrl::AbAttributeItem, std::pair<uint64_t, std::array<uint8_t, DhtStates::sk_keySizeByte> > > remoteItems;
			LocalAttrListCollecter(userId, neededAttrListPtr->GetList(), approvedList, remoteItems);

			if (remoteItems.size() == 0)
			{ //Can be answered now
				if (approvedList->size() == 0)
				{// Nothing new, directly return
					UserReadReturnWithoutCache(*tls, retVal, data);
				}
				else
				{//Has new attribute, retry
					UserReadReturnWithCache(*tls, dataKeyId, (*exListCachePtr) + (*approvedList));
				}

				return false;
			}

			//Remote Attr list collector
			std::shared_ptr<std::set<AccessCtrl::AbAttributeItem> > exList = std::make_shared<std::set<AccessCtrl::AbAttributeItem> >(exListCachePtr->GetList());
			
			std::shared_ptr<std::unique_ptr<EnclaveCntTranslator> > pendingCntPtr =
				std::make_shared<std::unique_ptr<EnclaveCntTranslator> >(Tools::make_unique<EnclaveCntTranslator>(std::move(cnt)));
			std::shared_ptr<std::unique_ptr<TlsCommLayer> > pendingTlsPtr =
				std::make_shared<std::unique_ptr<TlsCommLayer> >(std::move(tls));

			(*pendingTlsPtr)->SetConnectionPtr(*(*pendingCntPtr));

			auto callBackAfterCollect = [pendingCntPtr, pendingTlsPtr, dataKeyId](const AccessCtrl::AbAttributeList& exList, const AccessCtrl::AbAttributeList& approvedList) -> void*
			{
				void* res = nullptr;

				if (pendingCntPtr && pendingTlsPtr && *pendingCntPtr && *pendingTlsPtr)
				{
					if (approvedList.GetSize() == 0)
					{// Nothing new, directly return
						std::vector<uint8_t> data;
						UserReadReturnWithoutCache(*(*pendingTlsPtr), EncFunc::FileOpRet::k_denied, data);
					}
					else
					{//Has new attribute, retry
						UserReadReturnWithCache(*(*pendingTlsPtr), dataKeyId, exList + approvedList);
					}

					res = (*pendingCntPtr)->GetPointer();
					(*pendingTlsPtr).reset();
					(*pendingCntPtr).reset();
				}

				return res;
			};

			RemoteAttrListCollecter(userId, remoteItems, exList, approvedList, callBackAfterCollect);

			return true;
		}
		else
		{
			throw RuntimeException("Number of arguments for function k_readData doesn't match.");
		}

	case k_insertData:
		if (rpc.GetArgCount() == 4)
		{
			const auto& keyId = rpc.GetPrimitiveArg<uint8_t[DhtStates::sk_keySizeByte]>(); //Arg 2
			auto policyBin = rpc.GetBinaryArg(); //Arg 3
			auto dataBin = rpc.GetBinaryArg(); //Arg 4

			FullPolicy verifiedPolicy(policyBin.first, policyBin.second);
			std::vector<uint8_t> verifiedPolicyBin(verifiedPolicy.GetSerializedSize());
			verifiedPolicy.Serialize(verifiedPolicyBin.begin(), verifiedPolicyBin.end());

			uint8_t dataKeyId[DhtStates::sk_keySizeByte] = { 0 };
			Hasher<HashType::SHA256>().Calc(dataKeyId, gsk_appKeyIdPrefix, keyId);

			uint8_t retVal = InsertData(dataKeyId, verifiedPolicyBin.begin(), verifiedPolicyBin.end(), dataBin.first, dataBin.second);

			RpcWriter rpcReturned(RpcWriter::CalcSizePrim<decltype(retVal)>(), 1);
			auto rpcRetVal = rpcReturned.AddPrimitiveArg<decltype(retVal)>();

			rpcRetVal = retVal;

			tls->SendRpc(rpcReturned);

			return false;
		}
		else
		{
			throw RuntimeException("Number of arguments for function k_insertData doesn't match.");
		}

	case k_updateData:
		if (rpc.GetArgCount() == 5)
		{
			const auto& keyId = rpc.GetPrimitiveArg<uint8_t[DhtStates::sk_keySizeByte]>(); //Arg 2
			auto dataIn = rpc.GetBinaryArg(); //Arg 3
			const auto& validCache = rpc.GetPrimitiveArg<uint8_t>(); //Arg 4
			auto cacheBin = rpc.GetBinaryArg(); //Arg 5

			//User ID:
			std::string userKeyPem = tls->GetPublicKeyPem();
			general_256bit_hash userId = { 0 };
			Hasher<HashType::SHA256>().Calc(userId, userKeyPem);

			std::unique_ptr<AccessCtrl::AbAttributeList> exListCachePtr = ParseCachedAttList(validCache, cacheBin);
			std::unique_ptr<AccessCtrl::AbAttributeList> neededAttrListPtr = Tools::make_unique<AccessCtrl::AbAttributeList>();

			uint8_t dataKeyId[DhtStates::sk_keySizeByte] = { 0 };
			Hasher<HashType::SHA256>().Calc(dataKeyId, gsk_appKeyIdPrefix, keyId);

			uint8_t retVal = UpdateData(dataKeyId, *exListCachePtr, *neededAttrListPtr, dataIn.first, dataIn.second);

			if (retVal != EncFunc::FileOpRet::k_denied || neededAttrListPtr->GetSize() == 0)
			{ //Can return immediately
				UserUpdateReturnWithoutCache(*tls, retVal);

				return false;
			}

			//Start from here, retVal == denied && neededAttrList.size() > 0

			//Local Attr list collector
			std::shared_ptr<std::set<AccessCtrl::AbAttributeItem> > approvedList = std::make_shared<std::set<AccessCtrl::AbAttributeItem> >();
			std::map<AccessCtrl::AbAttributeItem, std::pair<uint64_t, std::array<uint8_t, DhtStates::sk_keySizeByte> > > remoteItems;
			LocalAttrListCollecter(userId, neededAttrListPtr->GetList(), approvedList, remoteItems);

			if (remoteItems.size() == 0)
			{ //Can be answered now
				if (approvedList->size() == 0)
				{// Nothing new, directly return
					UserUpdateReturnWithoutCache(*tls, retVal);
				}
				else
				{//Has new attribute, retry
					UserUpdateReturnWithCache(*tls, dataKeyId, (*exListCachePtr) + (*approvedList), std::vector<uint8_t>(dataIn.first, dataIn.second));
				}

				return false;
			}

			//Remote Attr list collector
			std::shared_ptr<std::set<AccessCtrl::AbAttributeItem> > exList = std::make_shared<std::set<AccessCtrl::AbAttributeItem> >(exListCachePtr->GetList());

			std::shared_ptr<std::unique_ptr<EnclaveCntTranslator> > pendingCntPtr =
				std::make_shared<std::unique_ptr<EnclaveCntTranslator> >(Tools::make_unique<EnclaveCntTranslator>(std::move(cnt)));
			std::shared_ptr<std::unique_ptr<TlsCommLayer> > pendingTlsPtr =
				std::make_shared<std::unique_ptr<TlsCommLayer> >(std::move(tls));

			(*pendingTlsPtr)->SetConnectionPtr(*(*pendingCntPtr));

			std::vector<uint8_t> data2UpdateLater(dataIn.first, dataIn.second);

			auto callBackAfterCollect = [pendingCntPtr, pendingTlsPtr, dataKeyId, data2UpdateLater](const AccessCtrl::AbAttributeList& exList, const AccessCtrl::AbAttributeList& approvedList) -> void*
			{
				void* res = nullptr;

				if (pendingCntPtr && pendingTlsPtr && *pendingCntPtr && *pendingTlsPtr)
				{
					if (approvedList.GetSize() == 0)
					{// Nothing new, directly return
						UserUpdateReturnWithoutCache(*(*pendingTlsPtr), EncFunc::FileOpRet::k_denied);
					}
					else
					{//Has new attribute, retry
						UserUpdateReturnWithCache(*(*pendingTlsPtr), dataKeyId, exList + approvedList, data2UpdateLater);
					}

					res = (*pendingCntPtr)->GetPointer();
					(*pendingTlsPtr).reset();
					(*pendingCntPtr).reset();
				}

				return res;
			};

			RemoteAttrListCollecter(userId, remoteItems, exList, approvedList, callBackAfterCollect);

			return true;
		}
		else
		{
			throw RuntimeException("Number of arguments for function k_readData doesn't match.");
		}

	case k_delData:
		if (rpc.GetArgCount() == 4)
		{
			const auto& keyId = rpc.GetPrimitiveArg<uint8_t[DhtStates::sk_keySizeByte]>(); //Arg 2
			const auto& validCache = rpc.GetPrimitiveArg<uint8_t>(); //Arg 3
			auto cacheBin = rpc.GetBinaryArg(); //Arg 4

			//User ID:
			std::string userKeyPem = tls->GetPublicKeyPem();
			general_256bit_hash userId = { 0 };
			Hasher<HashType::SHA256>().Calc(userId, userKeyPem);

			std::unique_ptr<AccessCtrl::AbAttributeList> exListCachePtr = ParseCachedAttList(validCache, cacheBin);
			std::unique_ptr<AccessCtrl::AbAttributeList> neededAttrListPtr = Tools::make_unique<AccessCtrl::AbAttributeList>();

			uint8_t dataKeyId[DhtStates::sk_keySizeByte] = { 0 };
			Hasher<HashType::SHA256>().Calc(dataKeyId, gsk_appKeyIdPrefix, keyId);

			uint8_t retVal = DelData(dataKeyId, *exListCachePtr, *neededAttrListPtr);

			if (retVal != EncFunc::FileOpRet::k_denied || neededAttrListPtr->GetSize() == 0)
			{ //Can return immediately
				UserDeleteReturnWithoutCache(*tls, retVal);

				return false;
			}

			//Start from here, retVal == denied && neededAttrList.size() > 0

			//Local Attr list collector
			std::shared_ptr<std::set<AccessCtrl::AbAttributeItem> > approvedList = std::make_shared<std::set<AccessCtrl::AbAttributeItem> >();
			std::map<AccessCtrl::AbAttributeItem, std::pair<uint64_t, std::array<uint8_t, DhtStates::sk_keySizeByte> > > remoteItems;
			LocalAttrListCollecter(userId, neededAttrListPtr->GetList(), approvedList, remoteItems);

			if (remoteItems.size() == 0)
			{ //Can be answered now
				if (approvedList->size() == 0)
				{// Nothing new, directly return
					UserDeleteReturnWithoutCache(*tls, retVal);
				}
				else
				{//Has new attribute, retry
					UserDeleteReturnWithCache(*tls, dataKeyId, (*exListCachePtr) + (*approvedList));
				}

				return false;
			}

			//Remote Attr list collector
			std::shared_ptr<std::set<AccessCtrl::AbAttributeItem> > exList = std::make_shared<std::set<AccessCtrl::AbAttributeItem> >(exListCachePtr->GetList());

			std::shared_ptr<std::unique_ptr<EnclaveCntTranslator> > pendingCntPtr =
				std::make_shared<std::unique_ptr<EnclaveCntTranslator> >(Tools::make_unique<EnclaveCntTranslator>(std::move(cnt)));
			std::shared_ptr<std::unique_ptr<TlsCommLayer> > pendingTlsPtr =
				std::make_shared<std::unique_ptr<TlsCommLayer> >(std::move(tls));

			(*pendingTlsPtr)->SetConnectionPtr(*(*pendingCntPtr));

			auto callBackAfterCollect = [pendingCntPtr, pendingTlsPtr, dataKeyId](const AccessCtrl::AbAttributeList& exList, const AccessCtrl::AbAttributeList& approvedList) -> void*
			{
				void* res = nullptr;

				if (pendingCntPtr && pendingTlsPtr && *pendingCntPtr && *pendingTlsPtr)
				{
					if (approvedList.GetSize() == 0)
					{// Nothing new, directly return
						UserDeleteReturnWithoutCache(*(*pendingTlsPtr), EncFunc::FileOpRet::k_denied);
					}
					else
					{//Has new attribute, retry
						UserDeleteReturnWithCache(*(*pendingTlsPtr), dataKeyId, exList + approvedList);
					}

					res = (*pendingCntPtr)->GetPointer();
					(*pendingTlsPtr).reset();
					(*pendingCntPtr).reset();
				}

				return res;
			};

			RemoteAttrListCollecter(userId, remoteItems, exList, approvedList, callBackAfterCollect);

			return true;
		}
		else
		{
			throw RuntimeException("Number of arguments for function k_readData doesn't match.");
		}

	case k_findAtListSuc:
		if (rpc.GetArgCount() == 2)
		{
			std::string listName = rpc.GetStringArg(); //Arg 2.

			//Calc User ID:
			std::string userKeyPem = tls->GetPublicKeyPem();
			general_256bit_hash userId = { 0 };
			Hasher<HashType::SHA256>().Calc(userId, userKeyPem);

			//Calc ListName ID:
			general_256bit_hash listId = { 0 };
			Hasher<HashType::SHA256>().Calc(listId, listName);

			uint8_t attrListId[DhtStates::sk_keySizeByte] = { 0 };
			Hasher<HashType::SHA256>().Calc(attrListId, gsk_atListKeyIdPrefix, userId, listId);

			return AppFindSuccessor(tls, cnt, attrListId);
		}
		else
		{
			throw RuntimeException("Number of arguments for function k_findAtListSuc doesn't match.");
		}

	case k_insertAttList:
		if (rpc.GetArgCount() == 3)
		{
			std::string listName = rpc.GetStringArg(); //Arg 2.
			auto listBin = rpc.GetBinaryArg(); //Arg 3.

			//Calc User ID:
			std::string userKeyPem = tls->GetPublicKeyPem();
			general_256bit_hash userId = { 0 };
			Hasher<HashType::SHA256>().Calc(userId, userKeyPem);
			//std::string userIdStr = cppcodec::base64_rfc4648::encode(userId);
			//PRINT_I("User, %s, is adding new attribute list.", userIdStr.c_str());

			//Calc ListName ID:
			general_256bit_hash listId = { 0 };
			Hasher<HashType::SHA256>().Calc(listId, listName);

			//Calc Attribute list ID:
			uint8_t attrListId[DhtStates::sk_keySizeByte] = { 0 };
			Hasher<HashType::SHA256>().Calc(attrListId, gsk_atListKeyIdPrefix, userId, listId);

			//Validate the given attribute list:
			AccessCtrl::EntityList attrListObj(listBin.first, listBin.second);

			std::vector<uint8_t> verifiedList(attrListObj.GetSerializedSize());
			attrListObj.Serialize(verifiedList.begin());

			//Construct access policy for attribute list:
			EntityItem owner(selfHashArray.m_h);
			std::unique_ptr<EntityList> entityList = Tools::make_unique<EntityList>();
			entityList->Insert(owner);

			EntityBasedControl entityCtrl(std::move(entityList), Tools::make_unique<EntityList>(), Tools::make_unique<EntityList>());
			FullPolicy listPolicy(owner, std::move(entityCtrl), AttributeBasedControl::DenyAll());

			std::vector<uint8_t> listPolicyBin(listPolicy.GetSerializedSize());
			listPolicy.Serialize(listPolicyBin.begin(), listPolicyBin.end());

			//Write attribute list:
			uint8_t retVal = InsertData(attrListId, listPolicyBin.begin(), listPolicyBin.end(), verifiedList.begin(), verifiedList.end());

			//Return result via RPC:
			RpcWriter rpcReturned(RpcWriter::CalcSizePrim<decltype(retVal)>(), 1);
			auto rpcRetVal = rpcReturned.AddPrimitiveArg<decltype(retVal)>();

			rpcRetVal = retVal;

			tls->SendRpc(rpcReturned);

			return false;
		}
		else
		{
			throw RuntimeException("Number of arguments for function k_findAtListSuc doesn't match.");
		}

	default:
		return false;
	}
}

bool Dht::AppFindSuccessor(std::unique_ptr<TlsCommLayer> & tls, Net::EnclaveCntTranslator& cnt, const uint8_t(&keyId)[DhtStates::sk_keySizeByte])
{
	ConstBigNumber queriedId(keyId);

	//LOGI("Recv app queried ID: %s.", static_cast<const BigNumber&>(queriedId).ToBigEndianHexStr().c_str());

	DhtStates::DhtLocalNodePtrType localNode = gs_state.GetDhtNode();

	uint64_t resAddr = 0;
	if (TryGetQueriedAddrLocally(*localNode, queriedId, resAddr))
	{
		//Query can be answered immediately.

		tls->SendStruct(resAddr);

		return false;
	}
	else
	{
		//Query should be forwarded to peer(s) and reply later.

		return AppFindSuccessorForwardProc(std::move(tls), cnt, keyId, queriedId, localNode);
	}
}
