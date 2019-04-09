#include <DecentApi/DecentAppEnclave/AppStates.h>
#include <DecentApi/DecentAppEnclave/AppStatesSingleton.h>
#include <DecentApi/Common/Ra/StatesSingleton.h>

#include <DecentApi/DecentAppEnclave/AppCertContainer.h>
#include <DecentApi/Common/Ra/KeyContainer.h>
#include <DecentApi/Common/Ra/WhiteList/Loaded.h>
#include <DecentApi/Common/Ra/WhiteList/DecentServer.h>

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

	static const WhiteList::Loaded& GetLoadedWhiteListImpl(WhiteList::Loaded* instPtr)
	{
		static const WhiteList::Loaded inst(instPtr);
		return inst;
	}

	static std::array<uint8_t, DhtStates::sk_keySizeByte> GetFilledArray()
	{
		std::array<uint8_t, DhtStates::sk_keySizeByte> filledArray;
		memset_s(filledArray.data(), filledArray.size(), 0xFF, filledArray.size());
		return filledArray;
	}

	static EnclaveStore& GetDhtStore()
	{
		static EnclaveStore inst(0, MbedTlsObj::ConstBigNumber(GetFilledArray()));

		return inst;
	}
}

DhtStates& Decent::Dht::GetDhtStatesSingleton()
{
	static DhtStates state(GetCertContainer(), GetKeyContainer(), GetServerWhiteList(), &GetLoadedWhiteListImpl, GetDhtStore());

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
