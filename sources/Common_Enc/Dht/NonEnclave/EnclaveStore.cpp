#ifdef ENCLAVE_PLATFORM_NON_ENCLAVE

#include "../EnclaveStore.h"

#include <DecentApi/Common/Common.h>
#include <DecentApi/Common/Net/RpcWriter.h>
#include <DecentApi/Common/Net/RpcParser.h>

#include "../../../Common/Dht/MemKeyValueStore.h"
#include "../../../Common/Dht/LocalNode.h"

#include "../DhtStatesSingleton.h"

using namespace Decent;
using namespace Decent::Dht;
using namespace Decent::Tools;

namespace
{
	DhtStates& gs_state = GetDhtStatesSingleton();

	static MemKeyValueStore::ValueType CombineMetaAndData(const std::vector<uint8_t>& meta, const std::vector<uint8_t>& data)
	{
		using namespace Net;

		RpcWriter rpc(
			RpcWriter::CalcSizeBin(meta.size()) +
			RpcWriter::CalcSizeBin(data.size()),
			2,
			false);

		auto metaBin = rpc.AddBinaryArg(meta.size());
		auto dataBin = rpc.AddBinaryArg(data.size());

		std::copy(meta.begin(), meta.end(), metaBin.begin());
		std::copy(data.begin(), data.end(), dataBin.begin());

		const auto& fnBin = rpc.GetFullBinary();

		MemKeyValueStore::ValueType val;
		val.first = fnBin.size();
		val.second = std::make_unique<uint8_t[]>(val.first);
		std::copy(fnBin.begin(), fnBin.end(), val.second.get());

		return std::move(val);
	}

	static void ParseMetaAndData(const MemKeyValueStore::ValueType& val, std::vector<uint8_t>& meta, std::vector<uint8_t>& data)
	{
		using namespace Net;

		RpcParser rpc(std::vector<uint8_t>(val.second.get(), val.second.get() + val.first));

		auto metaBin = rpc.GetBinaryArg();
		auto dataBin = rpc.GetBinaryArg();

		meta = std::vector<uint8_t>(metaBin.first, metaBin.second);
		data = std::vector<uint8_t>(dataBin.first, dataBin.second);
	}
}

EnclaveStore::EnclaveStore(const MbedTlsObj::BigNumber & ringStart, const MbedTlsObj::BigNumber & ringEnd) :
	StoreBase(ringStart, ringEnd),
	m_memStore(new MemKeyValueStore())
{}

EnclaveStore::~EnclaveStore()
{
	MemKeyValueStore* m_memStorePtr = static_cast<MemKeyValueStore*>(m_memStore);
	delete m_memStorePtr;
}

bool EnclaveStore::IsResponsibleFor(const MbedTlsObj::BigNumber & key) const
{
	DhtStates::DhtLocalNodePtrType localNode = gs_state.GetDhtNode();
	return localNode ? localNode->IsResponsibleFor(key) : false;
}

General128Tag EnclaveStore::SaveDataFile(const MbedTlsObj::BigNumber& key, const std::vector<uint8_t>& meta, const std::vector<uint8_t>& data)
{
	using namespace Decent::Tools;

	const std::string keyStr = key.ToBigEndianHexStr();
	LOGI("DHT store: adding key to the index. %s", keyStr.c_str());
	//LOGI("DHT store: writing value: %s", std::string(reinterpret_cast<const char*>(data.data()), data.size()).c_str());

	General128Tag tag = General128Tag();
	{
		MemKeyValueStore* m_memStorePtr = static_cast<MemKeyValueStore*>(m_memStore);

		m_memStorePtr->Store(keyStr, CombineMetaAndData(meta, data));
	}

	return tag;
}

void EnclaveStore::DeleteDataFile(const MbedTlsObj::BigNumber& key)
{
	const std::string keyStr = key.ToBigEndianHexStr();

	{
		MemKeyValueStore* m_memStorePtr = static_cast<MemKeyValueStore*>(m_memStore);

		MemKeyValueStore::ValueType val = m_memStorePtr->Delete(keyStr);

		if (val.second.get() != nullptr)
		{
			throw RuntimeException("Failed to delete key-value pair, " + keyStr + ". Pair not found.");
		}
	}
}

std::vector<uint8_t> EnclaveStore::ReadDataFile(const MbedTlsObj::BigNumber& key, const General128Tag& tag, std::vector<uint8_t>& meta)
{
	using namespace Decent::Tools;

	const std::string keyStr = key.ToBigEndianHexStr();

	{
		MemKeyValueStore* m_memStorePtr = static_cast<MemKeyValueStore*>(m_memStore);

		MemKeyValueStore::ValueType val = m_memStorePtr->Read(keyStr);

		std::vector<uint8_t> data;
		ParseMetaAndData(val, meta, data);

		return data;
	}
}

std::vector<uint8_t> EnclaveStore::MigrateOneDataFile(const MbedTlsObj::BigNumber& key, const General128Tag& tag, std::vector<uint8_t>& meta)
{
	using namespace Decent::Tools;

	const std::string keyStr = key.ToBigEndianHexStr();

	{
		MemKeyValueStore* m_memStorePtr = static_cast<MemKeyValueStore*>(m_memStore);

		MemKeyValueStore::ValueType val = m_memStorePtr->Delete(keyStr);

		std::vector<uint8_t> data;
		ParseMetaAndData(val, meta, data);

		return data;
	}
}

#endif //ENCLAVE_PLATFORM_NON_ENCLAVE
