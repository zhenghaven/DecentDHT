#ifdef ENCLAVE_PLATFORM_NON_ENCLAVE

#include <DecentApi/Common/MbedTls/MbedTlsCppDefs.h>

#include <DecentApi/CommonEnclave/Ra/TlsConfigSameEnclave.h>

#include <mbedtls/ssl.h>

using namespace Decent::Ra;

int TlsConfigSameEnclave::VerifyCert(mbedtls_x509_crt & cert, int depth, uint32_t & flag) const
{
	flag = MbedTlsObj::MBEDTLS_SUCCESS_RET;
	return MbedTlsObj::MBEDTLS_SUCCESS_RET;
}

int TlsConfigSameEnclave::VerifyDecentAppCert(const AppX509Cert & cert, int depth, uint32_t & flag) const
{
	flag = MbedTlsObj::MBEDTLS_SUCCESS_RET;
	return MbedTlsObj::MBEDTLS_SUCCESS_RET;
}

#endif //ENCLAVE_PLATFORM_NON_ENCLAVE
