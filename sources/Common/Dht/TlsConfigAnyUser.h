#pragma once

#include <DecentApi/Common/Ra/TlsConfigBase.h>

namespace Decent
{
	namespace Dht
	{
		/**
		 * \brief	The Decent RA TLS configuration that accept TLS connection with the peer that is the
		 * 			same enclave (i.e. same hash) as this one.
		 */
		class TlsConfigAnyUser : public Ra::TlsConfigBase
		{
		public:
			using TlsConfigBase::TlsConfigBase;

		protected:
			virtual int VerifyCert(mbedtls_x509_crt& cert, int depth, uint32_t& flag) const override;

			virtual int VerifyDecentAppCert(const Ra::AppX509Cert& cert, int depth, uint32_t& flag) const override;

		};
	}
}
