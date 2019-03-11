#pragma once

namespace Decent
{
	namespace Net
	{
		class TlsCommLayer;
	}

    namespace Dht
    {
		void DeUpdateFingerTable(void* connection, Decent::Net::TlsCommLayer &tls);
		void UpdateFingerTable(void* connection, Decent::Net::TlsCommLayer &tls);
		void GetImmediatePredecessor(void* connection, Decent::Net::TlsCommLayer &tls);
		void SetImmediatePredecessor(void* connection, Decent::Net::TlsCommLayer &tls);
		void GetImmediateSucessor(void* connection, Decent::Net::TlsCommLayer &tls);
		void GetNodeId(void* connection, Decent::Net::TlsCommLayer &tls);
		void FindPredecessor(void* connection, Decent::Net::TlsCommLayer &tls);
		void FindSuccessor(void* connection, Decent::Net::TlsCommLayer &tls);
		void ProcessDhtQueries(void* connection, Decent::Net::TlsCommLayer& tls);
    }
}
