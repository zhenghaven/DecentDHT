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
		class StoreBase
		{
		public: //static member:
			typedef std::set<IdType> IndexingType;

		public:
			StoreBase() = delete;

			StoreBase(const IdType& ringStart, const IdType& ringEnd) :
				m_ringStart(ringStart),
				m_ringEnd(ringEnd),
				m_indexing(),
				m_indexingMutex()
			{}

			virtual ~StoreBase()
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

			/**
			 * \brief	Sends migrating data to remote DHT store.
			 *
			 * \tparam	SendFuncT   	Type of the send function t. Must have the form of
			 * 							"void FuncName(const void* buf, size_t size)".
			 * \tparam	SendNumFuncT	Type of the send function t for sending key numbers.
			 * 							Must have the form of "void FuncName(const IdType& key)".
			 * \param	sendFunc		The send function for sending data.
			 * \param	sendNumFunc 	The send function for sending key number value.
			 * \param	sendIndexing	The indexing for data that need to be sent.
			 */
			template<typename SendFuncT, typename SendNumFuncT>
			void SendMigratingData(SendFuncT sendFunc, SendNumFuncT sendNumFunc, const IndexingType& sendIndexing)
			{
				uint64_t numOfData = static_cast<uint64_t>(sendIndexing.size());

				sendFunc(&numOfData, sizeof(numOfData)); //1. Send number of data to be sent.

				for (const IdType& index : sendIndexing)
				{
					sendNumFunc(index); //2. Send Key of the data.

					std::vector<uint8_t> data = GetAndDeleteDataFile(index);

					uint64_t sizeOfData = static_cast<uint64_t>(data.size());
					sendFunc(&sizeOfData, sizeof(sizeOfData)); //3. Send size of data to be sent.
					sendFunc(data.data(), data.size()); //4. Send data. - Done!
				}
			}

			/**
			 * \brief	Sends migrating data to remote DHT store.
			 *
			 * \tparam	SendFuncT   	Type of the send function t. Must have the form of 
			 * 							"void FuncName(const void* buf, size_t size)".
			 * \tparam	SendNumFuncT	Type of the send function t for sending key numbers. 
			 * 							Must have the form of "void FuncName(const IdType&amp; key)".
			 * \param	sendFunc   	The send function for sending data.
			 * \param	sendNumFunc	The send function for sending key number value.
			 * \param	start	   	The start position on the ring (INclusive).
			 * \param	end		   	The end position on the ring (EXclusive).
			 */
			template<typename SendFuncT, typename SendNumFuncT>
			void SendMigratingData(SendFuncT sendFunc, SendNumFuncT sendNumFunc, const IdType& start, const IdType& end)
			{
				SendMigratingData(sendFunc, sendNumFunc, DeleteIndexing(start, end));
			}

			/**
			 * \brief	Receive migrating data from remote DHT store.
			 *
			 * \tparam	RecvFuncT   	Type of the receive function t. Must have the form of
			 * 							"void FuncName(void* buf, size_t size)".
			 * \tparam	RecvNumFuncT	Type of the receive function t for receiving key number value.
			 * 							Must have the form of "IdType FuncName()"
			 * \param	recvFunc   	The receive function for receiving data.
			 * \param	recvNumFunc	The receive function for receiving key number value.
			 */
			template<typename RecvFuncT, typename RecvNumFuncT>
			void RecvMigratingData(RecvFuncT recvFunc, RecvNumFuncT recvNumFunc)
			{
				std::unique_lock<std::mutex> indexingLock(m_indexingMutex);

				uint64_t numOfData = 0;
				recvFunc(&numOfData, sizeof(numOfData)); //1. Receive number of data to be received.

				for (uint64_t i = 0; i < numOfData; ++i)
				{
					IdType key = recvNumFunc(); //2. Receive key of the data.

					uint64_t sizeOfData = 0;
					recvFunc(&sizeOfData, sizeof(sizeOfData)); //3. Receive size of data to be sent.

					std::vector<uint8_t> data(sizeOfData);
					recvFunc(data.data(), data.size()); //4. Receive data. - Done!

					SaveData(std::move(key), std::move(data));
				}
			}

			virtual bool IsResponsibleFor(const IdType& key) const = 0;

			virtual void SetValue(const IdType& key, std::vector<uint8_t>&& data)
			{
				if (IsResponsibleFor(key))
				{
					std::unique_lock<std::mutex> indexingLock(m_indexingMutex);
					SaveData(key, std::forward<std::vector<uint8_t> >(data));
				}
			}

			virtual void DelValue(const IdType& key)
			{
				if (IsResponsibleFor(key))
				{
					std::unique_lock<std::mutex> indexingLock(m_indexingMutex);
					m_indexing.erase(key);
					DeleteDataFile(key);
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
			 * \brief	Get the value specified by key, and then delete the data file from the file system.
			 * 			NOTE: this doesn't delete the key from indexing.
			 *
			 * \param	key	The key.
			 *
			 * \return	The data in std::vector&lt;uint8_t&gt;
			 */
			virtual std::vector<uint8_t> GetAndDeleteDataFile(const IdType& key)
			{
				std::vector<uint8_t> res;
				GetValue(key, res);

				DeleteDataFile(key);

				return res;
			}

			/**
			 * \brief	Delete the data file from the file system.
			 * 			NOTE: this doesn't delete the key from indexing.
			 *
			 * \param	key	The key.
			 *
			 */
			virtual void DeleteDataFile(const IdType& key) = 0;

			/**
			 * \brief	Adds key to the index if it doesn't exist. Saves the data to storage. Note: this
			 * 			function should not lock the indexing mutex.
			 *
			 * \param 		  	key 	The key.
			 * \param [in,out]	data	The data.
			 */
			virtual void SaveData(IdType&& key, std::vector<uint8_t>&& data)
			{
				m_indexing.insert(std::forward<IdType>(key));
			}

			/**
			 * \brief	Adds key to the index if it doesn't exist. Saves the data to storage. Note: this
			 * 			function should not lock the indexing mutex.
			 *
			 * \param 		  	key 	The key.
			 * \param [in,out]	data	The data.
			 */
			virtual void SaveData(const IdType& key, std::vector<uint8_t>&& data)
			{
				m_indexing.insert(key);
			}

		private:
			IdType m_ringStart;
			IdType m_ringEnd;
			IndexingType m_indexing;
			mutable std::mutex m_indexingMutex;
		};
	}
}
