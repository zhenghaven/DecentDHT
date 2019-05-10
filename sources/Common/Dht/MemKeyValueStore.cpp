#include "MemKeyValueStore.h"

#include <cstring>

//#include <DecentApi/Common/Common.h>
#include <DecentApi/Common/make_unique.h>

using namespace Decent::Dht;

MemKeyValueStore::MemKeyValueStore()
{
}

MemKeyValueStore::~MemKeyValueStore()
{
}

void MemKeyValueStore::Store(const KeyType & key, ValueType && val)
{
	std::unique_lock<std::mutex> mapLock(m_mapMutex);
	auto it = m_map.find(key);
	if (it != m_map.end())
	{
		//Assign:
		it = m_map.erase(it);
		m_map.insert(it, std::make_pair(key, std::forward<ValueType>(val)));
	}
	else
	{
		//Insert:
		m_map.insert(std::make_pair(key, std::forward<ValueType>(val)));
	}
	//PRINT_I("Num of value stored: %llu.", m_map.size());
}

MemKeyValueStore::ValueType MemKeyValueStore::Read(const KeyType & key)
{
	std::unique_lock<std::mutex> mapLock(m_mapMutex);
	auto it = m_map.find(key);
	if (it != m_map.end())
	{
		ValueType res;
		
		res.first = it->second.first;
		res.second = Tools::make_unique<uint8_t[]>(res.first);
		
		std::memcpy(res.second.get(), it->second.second.get(), res.first);

		return res;
	}
	else
	{
		//Val not found:
		return ValueType();
	}
}

MemKeyValueStore::ValueType MemKeyValueStore::Delete(const KeyType & key)
{
	std::unique_lock<std::mutex> mapLock(m_mapMutex);
	auto it = m_map.find(key); 
	if (it != m_map.end())
	{
		return Delete(it);
	}
	else
	{
		//Value not found.
		return ValueType();
	}
}

std::vector<MemKeyValueStore::KeyValPair> MemKeyValueStore::Migrate(const KeyType & lowerVal, const KeyType & higherVal)
{
	std::vector<MemKeyValueStore::KeyValPair> res;

	{
		std::unique_lock<std::mutex> mapLock(m_mapMutex);

		auto itBegin = m_map.lower_bound(lowerVal);
		auto itEnd = m_map.upper_bound(higherVal);

		for (auto it = itBegin; it != itEnd; it = m_map.erase(it))
		{
			res.push_back(std::make_pair(std::move(it->first), std::move(it->second)));
		}
	}

	return res;
}

std::vector<MemKeyValueStore::KeyValPair> MemKeyValueStore::MigrateAll()
{
	std::vector<MemKeyValueStore::KeyValPair> res;

	{
		std::unique_lock<std::mutex> mapLock(m_mapMutex);
		for (auto it = m_map.begin(); it != m_map.end(); it = m_map.erase(it))
		{
			res.push_back(std::make_pair(std::move(it->first), std::move(it->second)));
		}
	}

	return res;
}

MemKeyValueStore::ValueType MemKeyValueStore::Delete(MapType::iterator it)
{
	//Protected function; assume 'it' is not pointing to the end; assume map has been locked.
	ValueType res = std::move(it->second);

	m_map.erase(it);

	return std::move(res);
}
