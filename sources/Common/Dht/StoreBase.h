#pragma once

#include <cstdint>

#include <vector>
#include <map>
#include <mutex>

#include <DecentApi/Common/GeneralKeyTypes.h>
#include <DecentApi/Common/RuntimeException.h>

namespace Decent
{
	namespace Dht
	{
		class DataAlreadyExist : public RuntimeException
		{
		public:
			DataAlreadyExist() :
				RuntimeException("Data is already existing in DHT store, and override is not allowed.")
			{}
		};

		class DataNotExist : public RuntimeException
		{
		public:
			DataNotExist() :
				RuntimeException("Data is not existing in DHT store.")
			{}
		};

		template<typename IdType, typename AddrType>
		class StoreBase
		{
		public: //static member:
			typedef std::map<IdType, General128Tag > IndexingType;

		public:
			StoreBase() = delete;

			StoreBase(const IdType& ringStart, const IdType& ringEnd) :
				m_ringStart(ringStart),
				m_ringEnd(ringEnd),
				m_indexingMutex(),
				m_indexing()
			{}

			virtual ~StoreBase()
			{}

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
				static constexpr uint8_t hasData2Send = 1;
				static constexpr uint8_t noData2Send = 0;

				for (auto it = sendIndexing.begin(); it != sendIndexing.end(); ++it)
				{
					std::vector<uint8_t> meta;
					std::vector<uint8_t> data;
					try
					{
						data = MigrateOneDataFile(it->first, it->second, meta);
					}
					catch (const std::exception&)
					{
						continue;
					}
					uint64_t sizeOfMeta = static_cast<uint64_t>(meta.size());
					uint64_t sizeOfData = static_cast<uint64_t>(data.size());

					sendFunc(&hasData2Send, sizeof(hasData2Send)); //1. Yes, we have data to send.
					sendNumFunc(it->first);                        //2. Send Key of the data.

					sendFunc(&sizeOfMeta, sizeof(sizeOfMeta));     //3. Send size of metadata.
					sendFunc(meta.data(), meta.size());            //4. Send metadata.

					sendFunc(&sizeOfData, sizeof(sizeOfData));     //5. Send size of data.
					sendFunc(data.data(), data.size());            //6. Send data. - Done!
				}

				sendFunc(&noData2Send, sizeof(noData2Send));       //5. Stop.
			}

			/**
			 * \brief	Migrate a range of data to send to the remote DHT store.
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
			 * \brief	Migrates all data to send to the remote DHT store.
			 *
			 * \tparam	SendFuncT   	Type of the send function t. Must have the form of "void
			 * 							FuncName(const void* buf, size_t size)".
			 * \tparam	SendNumFuncT	Type of the send function t for sending key numbers. Must have the
			 * 							form of "void FuncName(const IdType&amp; key)".
			 * \param	sendFunc   	The send function for sending data.
			 * \param	sendNumFunc	The send function for sending key number value.
			 */
			template<typename SendFuncT, typename SendNumFuncT>
			void SendMigratingDataAll(SendFuncT sendFunc, SendNumFuncT sendNumFunc)
			{
				IndexingType indexing;
				{
					std::unique_lock<std::mutex> indexingLock(m_indexingMutex);
					indexing.swap(m_indexing);
				}
				SendMigratingData(sendFunc, sendNumFunc, indexing);
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
				static constexpr uint8_t hasData2Recv = 1;
				//static constexpr uint8_t noData2Recv = 0;

				uint8_t hasData = 0;
				recvFunc(&hasData, sizeof(hasData)); //1. Do we have data to receive?

				uint64_t sizeOfData = 0;
				uint64_t sizeOfMeta = 0;
				while (hasData == hasData2Recv)
				{
					IdType key = recvNumFunc();                //2. Receive key of the data.

					recvFunc(&sizeOfMeta, sizeof(sizeOfMeta)); //3. Receive size of metadata.
					std::vector<uint8_t> meta(sizeOfMeta);
					recvFunc(meta.data(), meta.size());        //4. Receive metadata.

					recvFunc(&sizeOfData, sizeof(sizeOfData)); //5. Receive size of data.
					std::vector<uint8_t> data(sizeOfData);
					recvFunc(data.data(), data.size());        //6. Receive data. - Done!

					try
					{
						SetValue(key, meta, data);
					}
					catch (const std::exception&)
					{}

					recvFunc(&hasData, sizeof(hasData));       //1. Do we have more data to receive?
				}
			}

			virtual bool IsResponsibleFor(const IdType& key) const = 0;

			virtual void SetValue(const IdType& key, const std::vector<uint8_t>& meta, const std::vector<uint8_t>& data, bool allowOverride = true)
			{
				if (!IsResponsibleFor(key))
				{
					throw Decent::RuntimeException("This server is not resposible for queried key.");
				}

				if (!allowOverride)
				{
					std::unique_lock<std::mutex> indexingLock(m_indexingMutex);
					if (m_indexing.find(key) != m_indexing.end())
					{
						throw DataAlreadyExist();
					}
				}

				General128Tag tag = SaveDataFile(key, meta, data);

				{
					std::unique_lock<std::mutex> indexingLock(m_indexingMutex);
					m_indexing[key] = std::move(tag);
				}
			}

			virtual void DelValue(const IdType& key)
			{
				if (!IsResponsibleFor(key))
				{
					throw Decent::RuntimeException("This server is not resposible for queried key.");
				}

				{
					std::unique_lock<std::mutex> indexingLock(m_indexingMutex);
					m_indexing.erase(key);
				}

				DeleteDataFile(key);
			}

			virtual std::vector<uint8_t> GetValue(const IdType& key, std::vector<uint8_t>& meta)
			{
				if (!IsResponsibleFor(key))
				{
					throw Decent::RuntimeException("This server is not resposible for queried key.");
				}

				General128Tag tag;
				{
					std::unique_lock<std::mutex> indexingLock(m_indexingMutex);
					auto it = m_indexing.find(key);
					if (it == m_indexing.end())
					{
						throw DataNotExist();
					}
					tag = it->second;
				}

				return ReadDataFile(key, tag, meta);
			}

		protected:

			/**
			 * \brief	Saves the data to storage. NOTE: this function should only interact with file system.
			 *
			 * \param	key 	The key.
			 * \param	data	The data.
			 *
			 * \return	The tag for the data stored in std::vector&lt;uint8_t&gt;. The tag is generated for
			 * 			verification later.
			 */
			virtual General128Tag SaveDataFile(const IdType& key, const std::vector<uint8_t>& meta, const std::vector<uint8_t>& data) = 0;

			/**
			 * \brief	Delete the data file from the file system. NOTE: this function should only interact
			 * 			with file system.
			 *
			 * \param	key	The key.
			 */
			virtual void DeleteDataFile(const IdType& key) = 0;

			/**
			 * \brief	Reads data from file system. NOTE: this function should only interact with file
			 * 			system.
			 *
			 * \exception	Decent::RuntimeException	Thrown when the data read is invalid.
			 *
			 * \param	key	The key.
			 * \param	tag	The tag used to verify the validity of the data.
			 *
			 * \return	The data.
			 */
			virtual std::vector<uint8_t> ReadDataFile(const IdType& key, const General128Tag& tag, std::vector<uint8_t>& meta) = 0;

			/**
			 * \brief	Migrate (i.e. read and delete) one key-value pair from the file system. NOTE: this
			 * 			function should only interact with file system.
			 *
			 * \exception	Decent::RuntimeException	Thrown when the data read is invalid.
			 *
			 * \param	key	The key.
			 * \param	tag	The tag used to verify the validity of the data.
			 *
			 * \return	The data in std::vector&lt;uint8_t&gt;
			 */
			virtual std::vector<uint8_t> MigrateOneDataFile(const IdType& key, const General128Tag& tag, std::vector<uint8_t>& meta) = 0;


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
				auto endIt = m_indexing.upper_bound(end);
				for (auto it = m_indexing.lower_bound(start); it != endIt; it = m_indexing.erase(it))
				{
					res.insert(*it);
				}
			}

		private:
			IdType m_ringStart;
			IdType m_ringEnd;

			mutable std::mutex m_indexingMutex;
			IndexingType m_indexing;
		};
	}
}
