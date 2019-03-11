#include "DhtStore.h"

#include "DhtStatesSingleton.h"

#include "../../Common/Dht/LocalNode.h"

using namespace Decent;
using namespace Decent::Dht;

namespace
{
	DhtStates& gs_state = GetDhtStatesSingleton();
}

DhtStore::~DhtStore()
{
}


bool DhtStore::IsResponsibleFor(const MbedTlsObj::BigNumber & key) const
{
	DhtStates::DhtLocalNodePtrType localNode = gs_state.GetDhtNode();
	return localNode ? localNode->IsResponsibleFor(key) : false;
}

void DhtStore::GetValue(const MbedTlsObj::BigNumber & key, std::vector<uint8_t>& data)
{
	//TODO: work with file system.
}

std::vector<uint8_t> DhtStore::DeleteData(const MbedTlsObj::BigNumber & key)
{
	//TODO: work with file system.
	return std::vector<uint8_t>();
}

void DhtStore::SaveData(const MbedTlsObj::BigNumber & key, std::vector<uint8_t>&& data)
{
	//TODO: work with file system.
}
