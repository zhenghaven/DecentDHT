#pragma once

namespace Decent
{
	namespace Net
	{
		class TlsCommLayer;
	}

    namespace Dht
    {
		void ProcessDhtQueries(void* connection, Decent::Net::TlsCommLayer& tls);
    }
}
