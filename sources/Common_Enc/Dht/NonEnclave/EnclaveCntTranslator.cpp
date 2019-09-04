#ifdef ENCLAVE_PLATFORM_NON_ENCLAVE

#include <DecentApi/CommonEnclave/Net/EnclaveCntTranslator.h>

using namespace Decent::Net;

size_t EnclaveCntTranslator::SendRaw(const void * const dataPtr, const size_t size)
{
	return static_cast<ConnectionBase*>(m_cntPtr)->SendRaw(dataPtr, size);
}

size_t EnclaveCntTranslator::RecvRaw(void * const bufPtr, const size_t size)
{
	return static_cast<ConnectionBase*>(m_cntPtr)->RecvRaw(bufPtr, size);
}

void EnclaveCntTranslator::SendPack(const void * const dataPtr, const size_t size)
{
	return static_cast<ConnectionBase*>(m_cntPtr)->SendPack(dataPtr, size);
}

size_t EnclaveCntTranslator::RecvPack(uint8_t*& dest)
{
	return static_cast<ConnectionBase*>(m_cntPtr)->RecvPack(dest);
}

std::vector<uint8_t> EnclaveCntTranslator::SendAndRecvPack(const void * const inData, const size_t inDataLen)
{
	return static_cast<ConnectionBase*>(m_cntPtr)->SendAndRecvPack(inData, inDataLen);
}

void EnclaveCntTranslator::Terminate() noexcept
{
	return static_cast<ConnectionBase*>(m_cntPtr)->Terminate();
}

#endif //ENCLAVE_PLATFORM_NON_ENCLAVE
