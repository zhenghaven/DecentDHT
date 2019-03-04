#include <mutex>
#include <memory>

#include <DecentApi/Common/Common.h>
#include <DecentApi/Common/make_unique.h>
#include <DecentApi/Common/Ra/TlsConfig.h>
#include <DecentApi/Common/Ra/KeyContainer.h>
#include <DecentApi/Common/Ra/CertContainer.h>
#include <DecentApi/Common/Net/TlsCommLayer.h>
#include <DecentApi/Common/Tools/JsonTools.h>
#include <DecentApi/DecentAppEnclave/AppStatesSingleton.h>

#include <rapidjson/document.h>
#include <cppcodec/base64_default_rfc4648.hpp>

#include "../Common/AppNames.h"

using namespace Decent::Dht;
using namespace Decent::Ra;
using namespace Decent::Net;

namespace
{
	static AppStates& gs_state = GetAppStateSingleton();
}

static void DecentDhtLoop(void* const connection, TlsCommLayer& pasTls)
{

}

extern "C" int ecall_decent_dht_loopup(void* const connection)
{
	LOGI("Processing DHT loopup request...");

	//std::shared_ptr<Decent::Ra::TlsConfig> pasTlsCfg = std::make_shared<Decent::Ra::TlsConfig>("NaN", gs_state, true);
	//Decent::Net::TlsCommLayer pasTls(connection, pasTlsCfg, false);

	//DecentDhtLoop();

	return false;
}
