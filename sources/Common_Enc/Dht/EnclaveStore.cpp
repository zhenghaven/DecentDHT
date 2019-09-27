#include "EnclaveStore.h"

#include <DecentApi/Common/Common.h>

#if defined(ENCLAVE_PLATFORM_NON_ENCLAVE) && !defined(ENCLAVE_PLATFORM_NON_ENCLAVE_EXACT_COMPARE)

#include <DecentApi/Common/Net/RpcParser.h>
#include <DecentApi/Common/Net/RpcWriter.h>

#else

#include <DecentApi/CommonEnclave/Tools/DataSealer.h>

#endif //defined(ENCLAVE_PLATFORM_NON_ENCLAVE) && !defined(ENCLAVE_PLATFORM_NON_ENCLAVE_EXACT_COMPARE)

#include "../../Common/Dht/LocalNode.h"

#include "DhtStatesSingleton.h"

using namespace Decent;
using namespace Decent::Dht;
using namespace Decent::Tools;

namespace
{
	constexpr char gsk_sealKeyLabel[] = "Decent_DHT_Data";

	DhtStates& gs_state = GetDhtStatesSingleton();
}

EnclaveStore::EnclaveStore(const MbedTlsObj::BigNumber & ringStart, const MbedTlsObj::BigNumber & ringEnd) :
	StoreBase(ringStart, ringEnd),
	m_memStore()
{}

EnclaveStore::~EnclaveStore()
{
}

bool EnclaveStore::IsResponsibleFor(const MbedTlsObj::BigNumber & key) const
{
	DhtStates::DhtLocalNodePtrType localNode = gs_state.GetDhtNode();
	return localNode ? localNode->IsResponsibleFor(key) : false;
}

Decent::General128Tag EnclaveStore::SaveDataFile(const MbedTlsObj::BigNumber& key, const std::vector<uint8_t>& meta, const std::vector<uint8_t>& data)
{
	using namespace Decent::Tools;

	const std::string keyStr = key.ToBigEndianHexStr();
	LOGI("DHT store: adding key to the index. %s", keyStr.c_str());
	//LOGI("DHT store: writing value: %s", std::string(reinterpret_cast<const char*>(data.data()), data.size()).c_str());

#if defined(ENCLAVE_PLATFORM_NON_ENCLAVE) && !defined(ENCLAVE_PLATFORM_NON_ENCLAVE_EXACT_COMPARE)

	using namespace Decent::Net;

	RpcWriter packedData(
		RpcWriter::CalcSizeBin(meta.size()) +
		RpcWriter::CalcSizeBin(data.size()),
		2, false);

	packedData.AddBinaryArg(meta.size()).Set(meta);
	packedData.AddBinaryArg(data.size()).Set(data);

	General128Tag tag{ 0 };
	const std::vector<uint8_t>& sealedData = packedData.GetFullBinary();

#else

	General128Tag tag;
	std::vector<uint8_t> sealedData = DataSealer::SealData(DataSealer::KeyPolicy::ByMrEnclave, gs_state, gsk_sealKeyLabel, meta, data, &tag);

#endif //defined(ENCLAVE_PLATFORM_NON_ENCLAVE) && !defined(ENCLAVE_PLATFORM_NON_ENCLAVE_EXACT_COMPARE)
	
	m_memStore.Save(keyStr, sealedData);

	return tag;
}

void EnclaveStore::DeleteDataFile(const MbedTlsObj::BigNumber& key)
{
	const std::string keyStr = key.ToBigEndianHexStr();

	m_memStore.Delete(keyStr);
}

std::vector<uint8_t> EnclaveStore::ReadDataFile(const MbedTlsObj::BigNumber& key, const General128Tag& tag, std::vector<uint8_t>& meta)
{
	const std::string keyStr = key.ToBigEndianHexStr();
	
	std::vector<uint8_t> sealedData = m_memStore.Read(keyStr);

#if defined(ENCLAVE_PLATFORM_NON_ENCLAVE) && !defined(ENCLAVE_PLATFORM_NON_ENCLAVE_EXACT_COMPARE)

	using namespace Net;

	RpcParser packedData(sealedData);

	auto metaBin = packedData.GetBinaryArg();
	auto dataBin = packedData.GetBinaryArg();

	meta = std::vector<uint8_t>(metaBin.first, metaBin.second);
	std::vector<uint8_t> data = std::vector<uint8_t>(dataBin.first, dataBin.second);

#else

	std::vector<uint8_t> data;
	DataSealer::UnsealData(DataSealer::KeyPolicy::ByMrEnclave, gs_state, gsk_sealKeyLabel, sealedData, meta, data, &tag);

#endif //defined(ENCLAVE_PLATFORM_NON_ENCLAVE) && !defined(ENCLAVE_PLATFORM_NON_ENCLAVE_EXACT_COMPARE)

	return data;
}

std::vector<uint8_t> EnclaveStore::MigrateOneDataFile(const MbedTlsObj::BigNumber& key, const General128Tag& tag, std::vector<uint8_t>& meta)
{
	const std::string keyStr = key.ToBigEndianHexStr();

	std::vector<uint8_t> sealedData = m_memStore.MigrateOne(keyStr);

#if defined(ENCLAVE_PLATFORM_NON_ENCLAVE) && !defined(ENCLAVE_PLATFORM_NON_ENCLAVE_EXACT_COMPARE)

	using namespace Net;

	RpcParser packedData(sealedData);

	auto metaBin = packedData.GetBinaryArg();
	auto dataBin = packedData.GetBinaryArg();

	meta = std::vector<uint8_t>(metaBin.first, metaBin.second);
	std::vector<uint8_t> data = std::vector<uint8_t>(dataBin.first, dataBin.second);

#else

	std::vector<uint8_t> data;
	DataSealer::UnsealData(DataSealer::KeyPolicy::ByMrEnclave, gs_state, gsk_sealKeyLabel, sealedData, meta, data, &tag);

#endif //defined(ENCLAVE_PLATFORM_NON_ENCLAVE) && !defined(ENCLAVE_PLATFORM_NON_ENCLAVE_EXACT_COMPARE)

	return data;
}
