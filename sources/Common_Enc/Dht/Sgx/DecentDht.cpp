#include "../DecentDht.h"

#include <DecentApi/DecentAppEnclave/AppStatesSingleton.h>

#include "../../../Common/Dht/FuncNums.h"

#include "AppNames.h"

using Decent::Dht;

extern "C" int ecall_decent_dht_proc_msg_from_dht(void* connection)
{
    using namespace EncFunc::Dht;

    LOGI("Processing message from user who have not signed up yet...");

    std::shared_ptr<Decent::Ra::TlsConfig> tlsCfg = std::make_shared<Decent::Ra::TlsConfig>(AppNames::sk_decentDHT, gs_state, true);
    Decent::Net::TlsCommLayer tls(connection, tlsCfg, true);

    std::string msgBuf;

    if (!tls.ReceiveMsg(connection, msgBuf) )
    {
        return false;
    }

    const NumType funcNum = EncFunc::GetNum<NumType>(msgBuf);

    LOGI("Request Function: %d.", funcNum);

    switch (funcNum)
    {
        case k_lookup:

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
