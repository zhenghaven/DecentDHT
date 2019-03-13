#include "../DhtServer.h"

#include <DecentApi/Common/Common.h>
#include <DecentApi/Common/Ra/TlsConfig.h>
#include <DecentApi/Common/Net/TlsCommLayer.h>

#include "../../../Common/Dht/AppNames.h"
#include "../DhtStatesSingleton.h"
#include "../EnclaveStore.h"

using namespace Decent;
using namespace Decent::Dht;

namespace
{
	DhtStates& gs_state = Dht::GetDhtStatesSingleton();
}

extern "C" int ecall_decent_dht_proc_msg_from_dht(void* connection)
{
	if (!gs_state.GetDhtNode())
	{
		LOGW("Local DHT Node had not been initialized yet!");
		return false;
	}

	std::shared_ptr<Decent::Ra::TlsConfig> tlsCfg = std::make_shared<Decent::Ra::TlsConfig>(AppNames::sk_decentDHT, gs_state, true);
	Decent::Net::TlsCommLayer tls(connection, tlsCfg, true);

	ProcessDhtQueries(connection, tls);

	return false;
}

extern "C" int ecall_decent_dht_proc_msg_from_store(void* connection)
{
	if (!gs_state.GetDhtNode())
	{
		LOGW("Local DHT Node had not been initialized yet!");
		return false;
	}

	std::shared_ptr<Decent::Ra::TlsConfig> tlsCfg = std::make_shared<Decent::Ra::TlsConfig>(AppNames::sk_decentDHT, gs_state, true);
	Decent::Net::TlsCommLayer tls(connection, tlsCfg, true);
	
	//gs_state.GetDhtStore().RecvMigratingData()

	return false;
}
