#pragma once

#include <cstdint>

namespace Decent
{
	namespace Net
	{
		class TlsCommLayer;
	}

    namespace Dht
    {
		//DHT node functions:
		
		void ProcessDhtQuery(Decent::Net::TlsCommLayer& tls);

		void GetNodeId(Decent::Net::TlsCommLayer &tls);

		void FindSuccessor(Decent::Net::TlsCommLayer &tls);

		void FindPredecessor(Decent::Net::TlsCommLayer &tls);

		void GetImmediateSucessor(Decent::Net::TlsCommLayer &tls);

		void GetImmediatePredecessor(Decent::Net::TlsCommLayer &tls);

		void SetImmediatePredecessor(Decent::Net::TlsCommLayer &tls);

		void UpdateFingerTable(Decent::Net::TlsCommLayer &tls);

		void DeUpdateFingerTable(Decent::Net::TlsCommLayer &tls);

		//DHT Store functions:
		
		void ProcessStoreRequest(Decent::Net::TlsCommLayer & tls);
		
		void GetMigrateData(Decent::Net::TlsCommLayer & tls);

		void SetMigrateData(Decent::Net::TlsCommLayer & tls);

		void SetData(Decent::Net::TlsCommLayer & tls);

		void GetData(Decent::Net::TlsCommLayer & tls);

		void DelData(Decent::Net::TlsCommLayer & tls);

		//(De-)Initialization functions:
		
		void Init(uint64_t selfAddr, int isFirstNode, uint64_t exAddr, size_t totalNode, size_t idx);

		void DeInit();

		//Requests from Apps:
		
		void ProcessAppRequest(Decent::Net::TlsCommLayer & tls);
    }
}
