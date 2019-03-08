#include "../DecentDht.h"

#include <DecentApi/DecentAppEnclave/AppStatesSingleton.h>

#include "../../../Common/Dht/FuncNums.h"
#include "../../../Common/Dht/AppNames.h"
#include "../../../Common/Dht/Node.h"
#include <DecentApi/Common/Ra/TlsConfig.h>
#include <DecentApi/Common/Net/TlsCommLayer.h>
#include <sgx_error.h>
#include <DecentApi/DecentAppEnclave/AppStatesSingleton.h>

using namespace Decent::Dht;
using namespace Decent;

static void lookup(void* connection, Decent::Net::TlsCommLayer &tls) {
	std::string msgBuf;
	if (!tls.ReceiveMsg(connection, msgBuf)) {
		return;
	}

	uint64_t result = 0;

	std::string sendBuff(sizeof(result), 0);
	*reinterpret_cast<uint64_t*>(&sendBuff[0]) = result; 

	tls.SendMsg(connection, sendBuff);
}

extern "C" int ecall_decent_dht_proc_msg_from_dht(void* connection)
{
    using namespace EncFunc::Dht;
	Ra::AppStates state = Ra::GetAppStateSingleton();

    std::shared_ptr<Decent::Ra::TlsConfig> tlsCfg = std::make_shared<Decent::Ra::TlsConfig>(AppNames::sk_decentDHT, state, true);
    Decent::Net::TlsCommLayer tls(connection, tlsCfg, true);

    std::string msgBuf;

    if (!tls.ReceiveMsg(connection, msgBuf) )
    {
        return false;
    }

    const NumType funcNum = EncFunc::GetNum<NumType>(msgBuf);

    switch (funcNum)
    {
        case k_lookup:
			lookup(connection, tls);
			break;

        case k_leave:

            break;
        case k_join:

            break;
        default:
            break;
    }

    return false;
}
