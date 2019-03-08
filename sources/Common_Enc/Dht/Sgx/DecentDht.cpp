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

	tls.ReceiveMsg(connection, msgBuf);

	uint64_t result = 0;

	tls.SendStruct(connection, result);
}

extern "C" int ecall_decent_dht_proc_msg_from_dht(void* connection)
{
    using namespace EncFunc::Dht;
	Ra::AppStates state = Ra::GetAppStateSingleton();

    std::shared_ptr<Decent::Ra::TlsConfig> tlsCfg = std::make_shared<Decent::Ra::TlsConfig>(AppNames::sk_decentDHT, state, true);
    Decent::Net::TlsCommLayer tls(connection, tlsCfg, true);

	NumType funcNum;
	tls.ReceiveStruct(connection, funcNum);

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
