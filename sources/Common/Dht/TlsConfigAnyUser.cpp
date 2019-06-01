#include "TlsConfigAnyUser.h"

using namespace Decent::Dht;

int TlsConfigAnyUser::VerifyCert(mbedtls_x509_crt & cert, int depth, uint32_t & flag) const
{
	flag = MbedTlsObj::MBEDTLS_SUCCESS_RET;
	return MbedTlsObj::MBEDTLS_SUCCESS_RET;
}

int TlsConfigAnyUser::VerifyDecentAppCert(const Decent::Ra::AppX509 & cert, int depth, uint32_t & flag) const
{
	flag = MbedTlsObj::MBEDTLS_SUCCESS_RET;
	return MbedTlsObj::MBEDTLS_SUCCESS_RET;
}
