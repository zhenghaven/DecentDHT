#include "TlsConfigAnyUser.h"

using namespace Decent::Dht;
using namespace Decent::Ra;
using namespace Decent::MbedTlsObj;

int TlsConfigAnyUser::VerifyCert(mbedtls_x509_crt & cert, int depth, uint32_t & flag) const
{
	flag = MBEDTLS_SUCCESS_RET;
	return MBEDTLS_SUCCESS_RET;
}

int TlsConfigAnyUser::VerifyDecentAppCert(const AppX509Cert & cert, int depth, uint32_t & flag) const
{
	flag = MBEDTLS_SUCCESS_RET;
	return MBEDTLS_SUCCESS_RET;
}
