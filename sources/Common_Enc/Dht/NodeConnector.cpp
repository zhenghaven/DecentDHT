#include "NodeConnector.h"

#include <DecentApi/Common/Common.h>
#include <DecentApi/Common/make_unique.h>
#include <DecentApi/Common/Net/RpcWriter.h>
#include <DecentApi/Common/Net/RpcParser.h>
#include <DecentApi/Common/Net/SecureCommLayer.h>

#include "../../Common/Dht/FuncNums.h"
#include "DhtStatesSingleton.h"
#include "DhtSecureConnectionMgr.h"

using namespace Decent;
using namespace Decent::Dht;
using namespace Decent::MbedTlsObj;
using namespace Decent::Ra;
using namespace Decent::Net;

namespace
{
	DhtStates& gs_state = GetDhtStatesSingleton();

	static char gsk_ack[] = "ACK";

	static NodeConnector::NodeBasePtr ReturnedNode(SecureCommLayer & comm)
	{
		RpcParser rpcReturned(comm.RecvContainer<std::vector<uint8_t> >());
		
		const auto& keyBin = rpcReturned.GetPrimitiveArg<uint8_t[DhtStates::sk_keySizeByte]>();
		const auto& addr = rpcReturned.GetPrimitiveArg<uint64_t>();

		return std::make_shared<NodeConnector>(addr, BigNumber(keyBin, true));
	}
}

void NodeConnector::SendNode(SecureCommLayer & comm, NodeBasePtr node)
{
	std::array<uint8_t, DhtStates::sk_keySizeByte> keyBin{};
	node->GetNodeId().ToBinary(keyBin);
	comm.SendRawAll(keyBin.data(), keyBin.size()); //1. Sent resultant ID

	uint64_t addr = node->GetAddress();
	comm.SendStruct(addr); //2. Sent Address - Done!

	//LOGI("Sent result ID: %s.", node->GetNodeId().ToBigEndianHexStr().c_str());
}

void NodeConnector::SendNode(ConnectionBase& connection, SecureCommLayer & comm, NodeBasePtr node)
{
	comm.SetConnectionPtr(connection);
	SendNode(comm, node);
}

void NodeConnector::SendAddress(Decent::Net::SecureCommLayer & comm, NodeBasePtr node)
{
	uint64_t addr = node->GetAddress();
	comm.SendStruct(addr); //1. Sent Address - Done!
}

void NodeConnector::SendAddress(Decent::Net::ConnectionBase & connection, Decent::Net::SecureCommLayer & comm, NodeBasePtr node)
{
	comm.SetConnectionPtr(connection);
	SendAddress(comm, node);
}

NodeConnector::NodeBasePtr NodeConnector::ReceiveNode(SecureCommLayer & comm)
{
	std::array<uint8_t, DhtStates::sk_keySizeByte> keyBin{};
	uint64_t addr;

	comm.RecvRawAll(keyBin.data(), keyBin.size()); //1. Receive ID
	comm.RecvStruct(addr); //2. Receive Address.

	//LOGI("Recv result ID: %s.", BigNumber(keyBin).ToBigEndianHexStr().c_str());
	
	return std::make_shared<NodeConnector>(addr, BigNumber(keyBin, true));
}

NodeConnector::NodeBasePtr NodeConnector::ReceiveNode(ConnectionBase& connection, SecureCommLayer & comm)
{
	comm.SetConnectionPtr(connection);
	return ReceiveNode(comm);
}

NodeConnector::NodeConnector(uint64_t address) :
	m_address(address),
	m_Id()
{}

NodeConnector::NodeConnector(uint64_t address, const MbedTlsObj::BigNumber & Id) :
	m_address(address),
	m_Id(Tools::make_unique<MbedTlsObj::BigNumber>(Id))
{
}

NodeConnector::NodeConnector(uint64_t address, MbedTlsObj::BigNumber && Id) :
	m_address(address),
	m_Id(Tools::make_unique<MbedTlsObj::BigNumber>(std::forward<MbedTlsObj::BigNumber>(Id)))
{
}

NodeConnector::~NodeConnector()
{
}

NodeConnector::NodeBasePtr NodeConnector::LookupTypeFunc(const MbedTlsObj::BigNumber & key, EncFunc::Dht::NumType type)
{
	CntPair cntPair = gs_state.GetConnectionMgr().GetNew(m_address, gs_state);

	constexpr size_t rpcSize = RpcWriter::CalcSizePrim<EncFunc::Dht::NumType>() +
		RpcWriter::CalcSizePrim<uint8_t[DhtStates::sk_keySizeByte]>();

	RpcWriter rpc(rpcSize, 2);

	auto funcNum = rpc.AddPrimitiveArg<EncFunc::Dht::NumType>();
	auto keyBin = rpc.AddPrimitiveArg<uint8_t[DhtStates::sk_keySizeByte]>();

	funcNum.Get() = type;
	key.ToBinary(keyBin.Get());

	cntPair.GetCommLayer().SendRpc(rpc);

	NodeConnector::NodeBasePtr res = ReturnedNode(cntPair.GetCommLayer());

	return res;
}

NodeConnector::NodeBasePtr NodeConnector::FindSuccessor(const BigNumber & key)
{
	//LOGI("Node Connector: Finding Successor of Node %s...", GetNodeId().ToBigEndianHexStr().c_str());
	using namespace EncFunc::Dht;
	return LookupTypeFunc(key, k_findSuccessor);
}

NodeConnector::NodeBasePtr NodeConnector::FindPredecessor(const MbedTlsObj::BigNumber & key)
{
	//LOGI("Node Connector: Finding Predecessor of Node %s...", GetNodeId().ToBigEndianHexStr().c_str());
	using namespace EncFunc::Dht;
	return LookupTypeFunc(key, k_findPredecessor);
}

NodeConnector::NodeBasePtr NodeConnector::GetImmediateSuccessor()
{
	//LOGI("Node Connector: Getting Immediate Successor of Node %s...", GetNodeId().ToBigEndianHexStr().c_str());
	using namespace EncFunc::Dht;

	CntPair cntPair = gs_state.GetConnectionMgr().GetNew(m_address, gs_state);

	RpcWriter rpc(RpcWriter::CalcSizePrim<EncFunc::Dht::NumType>(), 1);
	rpc.AddPrimitiveArg<EncFunc::Dht::NumType>().Get() = k_getImmediateSuc;
	cntPair.GetCommLayer().SendRpc(rpc);

	return ReturnedNode(cntPair.GetCommLayer());
}

NodeConnector::NodeBasePtr NodeConnector::GetImmediatePredecessor()
{
	//LOGI("Node Connector: Getting Immediate Predecessor of Node %s...", GetNodeId().ToBigEndianHexStr().c_str());
	using namespace EncFunc::Dht;

	CntPair cntPair = gs_state.GetConnectionMgr().GetNew(m_address, gs_state);

	RpcWriter rpc(RpcWriter::CalcSizePrim<EncFunc::Dht::NumType>(), 1);
	rpc.AddPrimitiveArg<EncFunc::Dht::NumType>().Get() = k_getImmediatePre;
	cntPair.GetCommLayer().SendRpc(rpc);

	return ReturnedNode(cntPair.GetCommLayer());
}

void NodeConnector::SetImmediatePredecessor(NodeBasePtr pred)
{
	//LOGI("Node Connector: Setting Immediate Predecessor of Node %s...", GetNodeId().ToBigEndianHexStr().c_str());
	using namespace EncFunc::Dht;

	CntPair cntPair = gs_state.GetConnectionMgr().GetNew(m_address, gs_state);

	RpcWriter rpc(RpcWriter::CalcSizePrim<EncFunc::Dht::NumType>() +
		RpcWriter::CalcSizePrim<uint8_t[DhtStates::sk_keySizeByte]>() +
		RpcWriter::CalcSizePrim<uint64_t>(), 3);
	
	auto funcNum = rpc.AddPrimitiveArg<EncFunc::Dht::NumType>();
	auto keyBin = rpc.AddPrimitiveArg<uint8_t[DhtStates::sk_keySizeByte]>();
	auto addr = rpc.AddPrimitiveArg<uint64_t>();

	funcNum = k_setImmediatePre;
	pred->GetNodeId().ToBinary(keyBin.Get());
	addr = pred->GetAddress();

	cntPair.GetCommLayer().SendRpc(rpc);

	RpcParser rpcReturned(cntPair.GetCommLayer().RecvContainer<std::vector<uint8_t> >());
}

void NodeConnector::UpdateFingerTable(NodeBasePtr & s, uint64_t i)
{
	//LOGI("Node Connector: Updating Finger Table of Node %s...", GetNodeId().ToBigEndianHexStr().c_str());
	using namespace EncFunc::Dht;

	CntPair cntPair = gs_state.GetConnectionMgr().GetNew(m_address, gs_state);

	RpcWriter rpc(RpcWriter::CalcSizePrim<EncFunc::Dht::NumType>() +
		RpcWriter::CalcSizePrim<uint8_t[DhtStates::sk_keySizeByte]>() +
		RpcWriter::CalcSizePrim<uint64_t>() +
		RpcWriter::CalcSizePrim<uint64_t>(), 4);

	auto funcNum = rpc.AddPrimitiveArg<EncFunc::Dht::NumType>();
	auto keyBin = rpc.AddPrimitiveArg<uint8_t[DhtStates::sk_keySizeByte]>();
	auto addr = rpc.AddPrimitiveArg<uint64_t>();
	auto i2BeSend = rpc.AddPrimitiveArg<uint64_t>();

	funcNum = k_updFingerTable;
	s->GetNodeId().ToBinary(keyBin.Get());
	addr = s->GetAddress();
	i2BeSend = i;

	cntPair.GetCommLayer().SendRpc(rpc);

	RpcParser rpcReturned(cntPair.GetCommLayer().RecvContainer<std::vector<uint8_t> >());
}

void NodeConnector::DeUpdateFingerTable(const MbedTlsObj::BigNumber & oldId, NodeBasePtr & succ, uint64_t i)
{
	//LOGI("Node Connector: De-Updating Finger Table of Node %s...", GetNodeId().ToBigEndianHexStr().c_str());
	using namespace EncFunc::Dht;

	CntPair cntPair = gs_state.GetConnectionMgr().GetNew(m_address, gs_state);

	RpcWriter rpc(RpcWriter::CalcSizePrim<EncFunc::Dht::NumType>() +
		RpcWriter::CalcSizePrim<uint8_t[DhtStates::sk_keySizeByte]>() +
		RpcWriter::CalcSizePrim<uint8_t[DhtStates::sk_keySizeByte]>() +
		RpcWriter::CalcSizePrim<uint64_t>() +
		RpcWriter::CalcSizePrim<uint64_t>(), 5);

	auto funcNum = rpc.AddPrimitiveArg<EncFunc::Dht::NumType>();
	auto oldKeyBin = rpc.AddPrimitiveArg<uint8_t[DhtStates::sk_keySizeByte]>();
	auto keyBin = rpc.AddPrimitiveArg<uint8_t[DhtStates::sk_keySizeByte]>();
	auto addr = rpc.AddPrimitiveArg<uint64_t>();
	auto i2BeSend = rpc.AddPrimitiveArg<uint64_t>();

	funcNum = k_dUpdFingerTable;
	oldId.ToBinary(oldKeyBin.Get());
	succ->GetNodeId().ToBinary(keyBin.Get());
	addr = succ->GetAddress();
	i2BeSend = i;

	cntPair.GetCommLayer().SendRpc(rpc);

	RpcParser rpcReturned(cntPair.GetCommLayer().RecvContainer<std::vector<uint8_t> >());
}

const BigNumber & NodeConnector::GetNodeId()
{
	if (m_Id)
	{
		return *m_Id; //We already record the ID previously.
	}

	//If not, we need to query the peer:
	//LOGI("Node Connector: Getting Node ID...");
	using namespace EncFunc::Dht;

	CntPair cntPair = gs_state.GetConnectionMgr().GetNew(m_address, gs_state);

	constexpr size_t rpcSize = RpcWriter::CalcSizePrim<EncFunc::Dht::NumType>();

	//Send RPC:
	{
		RpcWriter rpc(rpcSize, 1);

		auto funcType = rpc.AddPrimitiveArg<EncFunc::Dht::NumType>();
		funcType.Get() = k_getNodeId;

		cntPair.GetCommLayer().SendRpc(rpc);
	}
	
	//RPC return:
	RpcParser rpcReturn(cntPair.GetCommLayer().RecvContainer<std::vector<uint8_t> >());

	const auto& keyBin = rpcReturn.GetPrimitiveArg<uint8_t[DhtStates::sk_keySizeByte]>();

	m_Id = Tools::make_unique<BigNumber>(keyBin, true);
	//LOGI("Recv result ID: %s.", m_Id->ToBigEndianHexStr().c_str());

	return *m_Id;
}
