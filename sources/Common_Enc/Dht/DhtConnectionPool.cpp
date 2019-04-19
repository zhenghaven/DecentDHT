#include "DhtConnectionPool.h"

#include <DecentApi/Common/make_unique.h>
#include <DecentApi/Common/Net/TlsCommLayer.h>

#include <DecentApi/CommonEnclave/Ra/TlsConfigSameEnclave.h>
#include <DecentApi/CommonEnclave/Net/EnclaveNetConnector.h>

#include "DhtStates.h"
#include "ConnectionManager.h"

using namespace Decent;
using namespace Decent::Net;
using namespace Decent::Dht;

namespace
{
	static std::shared_ptr<Ra::TlsConfigSameEnclave> GetClientTlsConfigDhtNode(DhtStates& state)
	{
		static std::shared_ptr<Ra::TlsConfigSameEnclave> tlsCfg = std::make_shared<Ra::TlsConfigSameEnclave>(state, Ra::TlsConfig::Mode::ClientHasCert);
		return tlsCfg;
	}
}

DhtCntPair::DhtCntPair(std::unique_ptr<Net::EnclaveNetConnector>&& cnt, std::unique_ptr<Net::TlsCommLayer>&& tls) :
	m_cnt(std::forward<std::unique_ptr<EnclaveNetConnector> >(cnt)),
	m_tls(std::forward<std::unique_ptr<TlsCommLayer> >(tls))
{
}

DhtCntPair::DhtCntPair(std::unique_ptr<Net::EnclaveNetConnector>& cnt, std::unique_ptr<Net::TlsCommLayer>& tls) :
	m_cnt(std::move(cnt)),
	m_tls(std::move(tls))
{
}

DhtCntPair::DhtCntPair(DhtCntPair && rhs) :
	m_cnt(std::forward<std::unique_ptr<EnclaveNetConnector> >(rhs.m_cnt)),
	m_tls(std::forward<std::unique_ptr<TlsCommLayer> >(rhs.m_tls))
{
}

DhtCntPair::~DhtCntPair()
{
}

TlsCommLayer & DhtCntPair::GetTlsCommLayer()
{
	return *m_tls;
}

EnclaveNetConnector & DhtCntPair::GetConnection()
{
	return *m_cnt;
}

DhtCntPair & DhtCntPair::operator=(DhtCntPair && rhs)
{
	if (this != &rhs)
	{
		m_cnt = std::forward<std::unique_ptr<EnclaveNetConnector> >(rhs.m_cnt);
		m_tls = std::forward<std::unique_ptr<TlsCommLayer> >(rhs.m_tls);
	}

	return *this;
}

DhtCntPair & Decent::Dht::DhtCntPair::Swap(DhtCntPair & other)
{
	m_cnt.swap(other.m_cnt);
	m_tls.swap(other.m_tls);
	return *this;
}

DhtConnectionPool::DhtConnectionPool(size_t maxInCnt, size_t maxOutCnt) :
	TlsConnectionPool(maxInCnt, maxOutCnt)
{
}

DhtConnectionPool::~DhtConnectionPool()
{
}

void DhtConnectionPool::Put(uint64_t addr, DhtCntPair && cntPair)
{
	std::unique_lock<std::mutex> mapItemLock;
	MapItemType* itemPtr = nullptr;

	{//mapLock

		std::unique_lock<std::mutex> mapLock(m_mapMutex);

		itemPtr = &(m_map[addr]);

		//Start mapItemLock
		std::unique_lock<std::mutex> mapItemLockTmp(itemPtr->first);
		mapItemLock.swap(mapItemLockTmp);

	}//End mapLock
	
	MapItemType& item = *itemPtr;
	if (item.second.size() < GetMaxOutConnection())
	{
		try
		{
			//Decent keep-alive
			
			char serverQuery;
			cntPair.GetTlsCommLayer().ReceiveStruct(serverQuery);

			item.second.push_back(std::forward<DhtCntPair>(cntPair));
		}
		catch (const std::exception&)
		{

		}
	}
	
	//End mapItemLock
}

DhtCntPair DhtConnectionPool::GetNew(uint64_t addr, DhtStates & state)
{
	std::unique_ptr<EnclaveNetConnector> connection = ConnectionManager::GetConnection2DecentNode(addr);
	std::unique_ptr<TlsCommLayer> tls = Tools::make_unique<TlsCommLayer>(connection->Get(), GetClientTlsConfigDhtNode(state), true);
	return DhtCntPair(connection, tls);
}

DhtCntPair DhtConnectionPool::Get(uint64_t addr, DhtStates& state)
{
	{//mapLock

		std::unique_lock<std::mutex> mapLock(m_mapMutex);
		auto it = m_map.find(addr);

		if (it != m_map.end())
		{
			{//mapItemLock
				std::unique_lock<std::mutex> mapItemLock(it->second.first);
				switch (it->second.second.size())
				{
				case 0:
				{
					//Usually this case won't happen, but just to be safe.
					{
						std::unique_lock<std::mutex> mapItemLockTmp(std::move(mapItemLock));
					}
					m_map.erase(it);
					break; //To Establish a new connection.
				}

				case 1:
				{
					//One last connection left.
					DhtCntPair res = std::move(it->second.second.back());
					it->second.second.pop_back();
					{
						std::unique_lock<std::mutex> mapItemLockTmp(std::move(mapItemLock));
					}
					m_map.erase(it);
					res.GetTlsCommLayer().SendStruct('W');
					return std::move(res);
				}

				default:
				{
					DhtCntPair res = std::move(it->second.second.back());
					it->second.second.pop_back();
					res.GetTlsCommLayer().SendStruct('W');
					return std::move(res);
				}

				}

			}//End mapItemLock

		}

	}//End mapLock

	// No connection available
	// Establish a new connection then
	return GetNew(addr, state);
}
