#pragma once

#include "../../Common/Dht/Node.h"
#include "../Connector.h"
#include <DecentApi/Common/Ra/TlsConfig.h>
#include <DecentApi/Common/Net/TlsCommLayer.h>
#include <sgx_error.h>
#include "../../Common/Dht/AppNames.h"
#include "../../Common/Dht/FuncNums.h"
#include <DecentApi/DecentAppEnclave/AppStatesSingleton.h>

#ifdef __cplusplus
extern "C" {
#endif

	sgx_status_t ocall_decent_dht_cnt_mgr_get_dht(void** out_cnt_ptr, uint64_t address);

#ifdef __cplusplus
}
#endif /* __cplusplus */

namespace Decent
{
	namespace Dht
	{
		template<typename ConstIdType>
		class NodeConnector : public Node
		{
		public:
			NodeConnector() = delete;

			NodeConnector(uint64_t address) :
				m_address(address)
			{}

			virtual ~NodeConnector() 
			{}

			virtual uint64_t FindSuccessor(ConstIdType key) const override
			{
				Ra::AppStates state = Ra::GetAppStateSingleton();

				void* connection;
				ocall_decent_dht_cnt_mgr_get_dht(&connection, m_address);
				
				std::shared_ptr<Decent::Ra::TlsConfig> tlsCfg = std::make_shared<Decent::Ra::TlsConfig>(AppNames::sk_decentDHT, state, false);
				Decent::Net::TlsCommLayer tls(connection, tlsCfg, true);

				tls.SendMsg(connection, EncFunc::GetMsgString(EncFunc::Dht::k_lookup));
				tls.SendMsg(connection, key);

				std::string msgbuffer;

				tls.ReceiveMsg(connection, msgbuffer);

				uint64_t result = *reinterpret_cast<const uint64_t*>(msgbuffer.data());

				ocall_decent_dht_cnt_mgr_close_cnt(&connection);

				return result;
			}

		private:
			uint64_t m_address;

		};
	}
}
