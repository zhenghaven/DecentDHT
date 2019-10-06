#ifdef ENCLAVE_PLATFORM_NON_ENCLAVE

#include <DecentApi/Common/Ra/AppX509Cert.h>
#include <DecentApi/DecentAppEnclave/AppCertContainer.h>

#ifdef DECENT_THREAD_SAFETY_HIGH
#include <atomic>
#endif // DECENT_THREAD_SAFETY_HIGH

#include <DecentApi/Common/Ra/Crypto.h>

using namespace Decent::Ra;

AppCertContainer::AppCertContainer() noexcept
{
}

AppCertContainer::~AppCertContainer() noexcept
{
}

std::shared_ptr<const AppX509Cert> AppCertContainer::GetAppCert() const noexcept
{
#ifdef DECENT_THREAD_SAFETY_HIGH
	return std::atomic_load(&m_cert);
#else
	return m_cert;
#endif // DECENT_THREAD_SAFETY_HIGH
}

bool AppCertContainer::SetAppCert(std::shared_ptr<const AppX509Cert> cert) noexcept
{
	if (!CertContainer::SetCert(cert))
	{
		return false;
	}
#ifdef DECENT_THREAD_SAFETY_HIGH
	std::atomic_store(&m_cert, cert);
#else
	m_cert = cert;
#endif // DECENT_THREAD_SAFETY_HIGH

	return true;
}

#endif //ENCLAVE_PLATFORM_NON_ENCLAVE
