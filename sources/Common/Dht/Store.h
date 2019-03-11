#pragma once

#include <cstdint>

#include <vector>
#include <set>
#include <mutex>

namespace Decent
{
	namespace Dht
	{
		template<typename IdType, typename AddrType>
		class Store
		{
		public: //static member:
			typedef std::set<IdType> IndexingType;

		public:
			Store() = delete;

			Store(const IdType& ringStart, const IdType& ringEnd) :
				m_ringStart(ringStart),
				m_ringEnd(ringEnd),
				m_indexing(),
				m_indexingMutex()
			{}

			virtual ~Store()
			{}

			/**
			 * \brief	Migrate data from remote DHT store.
			 *
			 * \param	addr 	The address of remote DHT store.
			 * \param	start	The start of the ID range (smallest value, exclusive).
			 * \param	end  	The end of the ID range (largest value, inclusive).
			 */
			virtual void MigrateFrom(const AddrType& addr, const IdType& start, const IdType& end) = 0;

			/**
			 * \brief	Migrate data to remote DHT store.
			 *
			 * \param	addr	The address of remote DHT store.
			 */
			virtual void MigrateTo(const AddrType& addr) = 0;

			virtual bool IsResponsibleFor(const IdType& key) const = 0;

			virtual void SetValue(const IdType& key, std::vector<uint8_t>&& data)
			{
				if (IsResponsibleFor(key))
				{
					std::unique_lock<std::mutex> indexingLock(m_indexingMutex);
					SaveData(key, std::forward<std::vector<uint8_t> >(data));
				}
			}

			virtual void GetValue(const IdType& key, std::vector<uint8_t>& data) = 0;

			//virtual void LockDataStore() = 0;

			//virtual void UnlockDataStore() = 0;

		protected:
			IndexingType& GetIndexing()
			{
				return m_indexing;
			}

			const IndexingType& GetIndexing() const
			{
				return m_indexing;
			}

			std::mutex& GetIndexingMutex() const
			{
				return m_indexingMutex;
			}

			/**
			 * \brief	Deletes the indexing within the specified range.
			 *
			 * \param	start	The start position on the ring (INclusive).
			 * \param	end  	The end position on the ring (EXclusive).
			 *
			 * \return	Result of deleted elements.
			 */
			virtual IndexingType DeleteIndexing(const IdType& start, const IdType& end)
			{
				std::unique_lock<std::mutex> indexingLock(m_indexingMutex);

				IndexingType res;
				if (start > end)
				{//normal case.
					DeleteIndexingNormalOrder(res, end + 1, start);
				}
				else if (end > start)
				{
					if (end != m_ringEnd)
					{
						DeleteIndexingNormalOrder(res, end + 1, m_ringEnd);
					}
					DeleteIndexingNormalOrder(res, m_ringStart, start);
				}

				return std::move(res);
			}

			/**
			 * \brief	Deletes the indexing within the specified range.
			 *
			 * \param [in,out]	res  	The result of deleted elements.
			 * \param 		  	start	The start of the ID range (smallest value, INclusive).
			 * \param 		  	end  	The end of the ID range (largest value, INclusive).
			 */
			virtual void DeleteIndexingNormalOrder(IndexingType& res, const IdType& start, const IdType& end)
			{
				for (auto it = m_indexing.lower_bound(start); it != m_indexing.end() && *it <= end; it = m_indexing.erase(it))
				{
					res.insert(*it);
				}
			}

			/**
			 * \brief	Deletes the data specified by key
			 *
			 * \param	key	The key.
			 *
			 * \return	The data in std::vector&lt;uint8_t&gt;
			 */
			virtual std::vector<uint8_t> DeleteData(const IdType& key) = 0;

			/**
			 * \brief	Adds key to the index if it doesn't exist. Saves the data to storage. Note: this
			 * 			function should not lock the indexing mutex.
			 *
			 * \param 		  	key 	The key.
			 * \param [in,out]	data	The data.
			 */
			virtual void SaveData(const IdType& key, std::vector<uint8_t>&& data) = 0;

		private:
			IdType m_ringStart;
			IdType m_ringEnd;
			IndexingType m_indexing;
			mutable std::mutex m_indexingMutex;
		};
	}
}
