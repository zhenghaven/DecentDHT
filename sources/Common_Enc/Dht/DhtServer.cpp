#include "DhtServer.h"

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
#include "DhtConnectionPool.h"
#include "DhtStatesSingleton.h"

using namespace Decent;
using namespace Decent::Net;
using namespace Decent::Dht;
using namespace Decent::MbedTlsObj;

namespace
{
	DhtStates& gs_state = Dht::GetDhtStatesSingleton();

	static char gsk_ack[] = "ACK";

	static std::mutex gs_clientPendingQueriesMutex;
	static std::map<std::string, std::pair<void*, CntPair> > gs_clientPendingQueries;

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
	std::string requestId;
	std::array<uint8_t, DhtStates::sk_keySizeByte> keyBin{};
	uint64_t reAddr = 0;

	tls.ReceiveMsg(requestId);                    //1. Receive request UUID
	tls.ReceiveRaw(keyBin.data(), keyBin.size()); //2. Receive queried ID
	tls.ReceiveStruct(reAddr);                    //3. Receive RE address

	ConstBigNumber queriedId(keyBin);

	DhtStates::DhtLocalNodePtrType localNode = gs_state.GetDhtNode();

	uint64_t resAddr = 0;
	if (TryGetQueriedAddrLocally(*localNode, queriedId, resAddr))
	{
		//Query can be answered immediately.
		//PRINT_I("Reply query with ID %s.", requestId.c_str());

		CntPair peerCntPair = gs_state.GetConnectionPool().GetNew(reAddr, gs_state);

		peerCntPair.GetCommLayer().SendStruct(EncFunc::Dht::k_queryReply);
		peerCntPair.GetCommLayer().SendMsg(requestId);   //1. Reply request UUID
		peerCntPair.GetCommLayer().SendStruct(resAddr);  //2. Reply result address
		return;
	}
	else
	{
		//Query has to be forwarded to other peer.
		//PRINT_I("Forward query with ID %s to other peer.", requestId.c_str());
		//Connect Peer:
		DhtStates::DhtLocalNodeType::NodeBasePtr nextHop = localNode->GetNextHop(queriedId);

		CntPair peerCntPair = gs_state.GetConnectionPool().GetNew(nextHop->GetAddress(), gs_state);

		peerCntPair.GetCommLayer().SendStruct(EncFunc::Dht::k_queryNonBlock);
		peerCntPair.GetCommLayer().SendMsg(requestId);                    //1. Send request UUID
		peerCntPair.GetCommLayer().SendRaw(keyBin.data(), keyBin.size()); //2. Forward queried key ID
		peerCntPair.GetCommLayer().SendStruct(reAddr);                    //3. Send RE addr

		return;
	}
}

void Dht::QueryReply(Decent::Net::TlsCommLayer & tls, void*& heldCntPtr)
{
	std::string requestId;
	uint64_t resAddr = 0;
	tls.ReceiveMsg(requestId);   //1. Receive request UUID
	tls.ReceiveStruct(resAddr);  //2. Receive result address

	//PRINT_I("Received forwarded query reply with ID %s.", requestId.c_str());

	std::unique_lock<std::mutex> clientPendingQueriesLock(gs_clientPendingQueriesMutex);
	auto it = gs_clientPendingQueries.find(requestId);
	if (it != gs_clientPendingQueries.end())
	{
		//Yes, it is. Reply to client.

		it->second.second.GetCommLayer().SendStruct(resAddr);
		heldCntPtr = it->second.first;

		gs_clientPendingQueries.erase(it);

		return;
	}
	else
	{
		LOGW("Pending request UUID is not found!");
	}
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
	memset_s(filledArray.data(), filledArray.size(), 0xFF, filledArray.size());
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
	std::array<uint8_t, DhtStates::sk_keySizeByte> keyBin{};
	tls.ReceiveRaw(keyBin.data(), keyBin.size()); //2. Received queried ID
	ConstBigNumber queriedId(keyBin);

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
		
		//UUID:
		Drbg drbg;
		std::array<uint8_t, GENERAL_128BIT_16BYTE_SIZE> uuidBin{};
		drbg.RandContainer(uuidBin);
		std::string requestId = cppcodec::base64_rfc4648::encode(uuidBin);
		//PRINT_I("Forward query with ID %s to peer.", requestId.c_str());

		//Add query to pending list.
		void* clientCntPtr = cnt.GetPointer();
		CntPair clientCntPair(Tools::make_unique<EnclaveCntTranslator>(std::move(cnt)), 
			Tools::make_unique<TlsCommLayer>(std::move(tls)));
		clientCntPair.GetCommLayer().SetConnectionPtr(clientCntPair.GetConnection());
		{
			std::unique_lock<std::mutex> clientPendingQueriesLock(gs_clientPendingQueriesMutex);
			gs_clientPendingQueries.insert(
				std::make_pair(requestId, 
					std::make_pair(clientCntPtr, std::move(clientCntPair))));
		}
		
		//Connect Peer:
		DhtStates::DhtLocalNodeType::NodeBasePtr nextHop = localNode->GetNextHop(queriedId);

		CntPair peerCntPair = gs_state.GetConnectionPool().GetNew(nextHop->GetAddress(), gs_state);

		peerCntPair.GetCommLayer().SendStruct(EncFunc::Dht::k_queryNonBlock);
		peerCntPair.GetCommLayer().SendMsg(requestId);                    //1. Send request UUID
		peerCntPair.GetCommLayer().SendRaw(keyBin.data(), keyBin.size()); //2. Forward queried key ID
		peerCntPair.GetCommLayer().SendStruct(localNode->GetAddress());   //3. Send RE addr

		return true;
	}
}
