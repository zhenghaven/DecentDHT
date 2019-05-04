#include "DhtConnectionPool.h"

#include <DecentApi/CommonApp/Net/TCPConnection.h>

using namespace Decent::Net;
using namespace Decent::Dht;

std::unique_ptr<ConnectionBase> DhtConnectionPool::GetNew(const uint64_t & addr)
{
	return std::make_unique<TCPConnection>(addr);
}
