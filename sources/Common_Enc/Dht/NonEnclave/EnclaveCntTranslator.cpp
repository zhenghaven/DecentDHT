#ifdef ENCLAVE_PLATFORM_NON_ENCLAVE

#include <DecentApi/CommonEnclave/Net/EnclaveCntTranslator.h>

using namespace Decent::Net;

size_t EnclaveCntTranslator::SendRaw(const void * const dataPtr, const size_t size)
{
	return static_cast<ConnectionBase*>(m_cntPtr)->SendRaw(dataPtr, size);
}

void EnclaveCntTranslator::SendPack(const void * const dataPtr, const size_t size)
{
	return static_cast<ConnectionBase*>(m_cntPtr)->SendPack(dataPtr, size);
}

size_t EnclaveCntTranslator::ReceiveRaw(void * const bufPtr, const size_t size)
{
	return static_cast<ConnectionBase*>(m_cntPtr)->ReceiveRaw(bufPtr, size);
}

void EnclaveCntTranslator::ReceivePack(std::string & outMsg)
{
	return static_cast<ConnectionBase*>(m_cntPtr)->ReceivePack(outMsg);
}

void EnclaveCntTranslator::ReceivePack(std::vector<uint8_t>& outMsg)
{
	return static_cast<ConnectionBase*>(m_cntPtr)->ReceivePack(outMsg);
}

void EnclaveCntTranslator::SendAndReceivePack(const void * const inData, const size_t inDataLen, std::string & outMsg)
{
	return static_cast<ConnectionBase*>(m_cntPtr)->SendAndReceivePack(inData, inDataLen, outMsg);
}

void EnclaveCntTranslator::Terminate() noexcept
{
	return static_cast<ConnectionBase*>(m_cntPtr)->Terminate();
}

#endif //ENCLAVE_PLATFORM_NON_ENCLAVE
