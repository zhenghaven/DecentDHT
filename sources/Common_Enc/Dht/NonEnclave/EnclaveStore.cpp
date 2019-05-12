#ifdef ENCLAVE_PLATFORM_NON_ENCLAVE

#include "../EnclaveStore.h"

#include <DecentApi/Common/Common.h>

#include "../../../Common/Dht/MemKeyValueStore.h"
#include "../../../Common/Dht/LocalNode.h"

#include "../DhtStatesSingleton.h"

using namespace Decent;
using namespace Decent::Dht;
using namespace Decent::Tools;

namespace
{
	DhtStates& gs_state = GetDhtStatesSingleton();

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

std::vector<uint8_t> EnclaveStore::SaveDataFile(const MbedTlsObj::BigNumber& key, const std::vector<uint8_t>& data)
{
	using namespace Decent::Tools;

	const std::string keyStr = key.ToBigEndianHexStr();
	LOGI("DHT store: adding key to the index. %s", keyStr.c_str());
	//LOGI("DHT store: writing value: %s", std::string(reinterpret_cast<const char*>(data.data()), data.size()).c_str());

	std::vector<uint8_t> mac;
	{
		MemKeyValueStore* m_memStorePtr = static_cast<MemKeyValueStore*>(m_memStore);

		MemKeyValueStore::ValueType val;
		val.first = data.size();
		val.second = std::make_unique<uint8_t[]>(val.first);
		std::copy(data.data(), data.data() + val.first, val.second.get());

		m_memStorePtr->Store(keyStr, std::move(val));
	}

	return mac;
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

std::vector<uint8_t> EnclaveStore::ReadDataFile(const MbedTlsObj::BigNumber& key, const std::vector<uint8_t>& tag)
{
	using namespace Decent::Tools;

	const std::string keyStr = key.ToBigEndianHexStr();

	{
		MemKeyValueStore* m_memStorePtr = static_cast<MemKeyValueStore*>(m_memStore);

		MemKeyValueStore::ValueType val = m_memStorePtr->Read(keyStr);

		return std::vector<uint8_t>(val.second.get(), val.second.get() + val.first);
	}
}

std::vector<uint8_t> EnclaveStore::MigrateOneDataFile(const MbedTlsObj::BigNumber& key, const std::vector<uint8_t>& tag)
{
	using namespace Decent::Tools;

	const std::string keyStr = key.ToBigEndianHexStr();

	{
		MemKeyValueStore* m_memStorePtr = static_cast<MemKeyValueStore*>(m_memStore);

		MemKeyValueStore::ValueType val = m_memStorePtr->Delete(keyStr);

		return std::vector<uint8_t>(val.second.get(), val.second.get() + val.first);
	}
}

#endif //ENCLAVE_PLATFORM_NON_ENCLAVE
