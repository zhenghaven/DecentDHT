#include <DecentApi/DecentAppEnclave/AppStates.h>
#include <DecentApi/DecentAppEnclave/AppStatesSingleton.h>
#include <DecentApi/Common/Ra/StatesSingleton.h>

#include <DecentApi/DecentAppEnclave/AppCertContainer.h>
#include <DecentApi/Common/Ra/KeyContainer.h>
#include <DecentApi/Common/Ra/WhiteList/Loaded.h>
#include <DecentApi/Common/Ra/WhiteList/HardCoded.h>
#include <DecentApi/Common/Ra/WhiteList/DecentServer.h>

#include "../Common/Dht/CircularRange.h"
#include "../Common_Enc/Dht/DhtStates.h"
#include "../Common_Enc/Dht/EnclaveStore.h"
#include "../Common_Enc/Dht/DhtStatesSingleton.h"

using namespace Decent;
using namespace Decent::Ra;
using namespace Decent::Dht;

namespace
{
	static AppCertContainer& GetCertContainer()
	{
		static AppCertContainer inst;
		return inst;
	}

	static KeyContainer& GetKeyContainer()
	{
		static KeyContainer inst;
		return inst;
	}

	static WhiteList::DecentServer& GetServerWhiteList()
	{
		static WhiteList::DecentServer inst;
		return inst;
	}

	static const WhiteList::HardCoded& GetHardCodedWhiteList()
	{
		static const WhiteList::HardCoded inst;
		return inst;
	}

	static const WhiteList::Loaded& GetLoadedWhiteListImpl(WhiteList::Loaded* instPtr)
	{
		static const WhiteList::Loaded inst(instPtr);
		return inst;
	}

	static EnclaveStore& GetDhtStore()
	{
		static EnclaveStore inst(MbedTlsObj::BigNumber(0LL, MbedTlsObj::sk_struct), MbedTlsObj::BigNumber(FilledByteArray<32>::value, MbedTlsObj::sk_struct));

		return inst;
	}
}

DhtStates& Decent::Dht::GetDhtStatesSingleton()
{
	static DhtStates state(GetCertContainer(), GetKeyContainer(), GetServerWhiteList(), GetHardCodedWhiteList(), &GetLoadedWhiteListImpl, GetDhtStore());

	return state;
}

AppStates& Decent::Ra::GetAppStateSingleton()
{
	return GetDhtStatesSingleton();
}

States& Decent::Ra::GetStateSingleton()
{
	return GetAppStateSingleton();
}
