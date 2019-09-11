#ifdef ENCLAVE_PLATFORM_NON_ENCLAVE

#include <DecentApi/Common/MbedTls/Kdf.h>

#include <DecentApi/CommonEnclave/Tools/DataSealer.h>

using namespace Decent::Tools;

namespace
{
	constexpr char gsk_sgxMetaLabel[] = "SGX";
}

void DataSealer::detail::DeriveSealKey(KeyPolicy keyPolicy, const Decent::Ra::States& decentState, const std::string& label,
	void* outKey, const size_t expectedKeySize, const void* inMeta, const size_t inMetaSize, const std::vector<uint8_t>& salt)
{
	G128BitSecretKeyWrap rootSealKey; //Non-enclave environment, we use all zeros for the root seal key.

	using namespace Decent::MbedTlsObj;

	HKDF<HashType::SHA256>(rootSealKey.m_key, label, salt, outKey, expectedKeySize);
}

std::vector<uint8_t> DataSealer::GenSealKeyRecoverMeta(bool isDefault)
{
	return std::vector<uint8_t>();
}

#endif //ENCLAVE_PLATFORM_NON_ENCLAVE
