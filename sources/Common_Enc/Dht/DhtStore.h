#pragma once

#include "../../Common/Dht/Store.h"

#include <DecentApi/Common/MbedTls/BigNumber.h>

#include "DhtStates.h"

namespace Decent
{
	namespace Dht
	{
		class DhtStore : public StoreBase<MbedTlsObj::BigNumber, uint64_t>
		{
		public:
			DhtStore(const MbedTlsObj::BigNumber& ringStart, const MbedTlsObj::BigNumber& ringEnd) :
				StoreBase(ringStart, ringEnd)
			{}

			virtual ~DhtStore();

			void MigrateFrom(const uint64_t& addr, const MbedTlsObj::BigNumber& start, const MbedTlsObj::BigNumber& end) override;

			virtual void MigrateTo(const uint64_t& addr) override;

			/**
			* \brief	Sends migrating data to remote DHT store.
			*
			* \tparam	SendFuncT	Type of the send function t. Must have the form of "void FuncName(const void* buf, size_t size)".
			* \param	sendFunc	The send function.
			* \param	start   	The start position on the ring (INclusive).
			* \param	end			The end position on the ring (EXclusive).
			*/
			template<typename SendFuncT>
			void SendMigratingData(SendFuncT sendFunc, const IndexingType& sendIndexing)
			{
				uint64_t numOfData = static_cast<uint64_t>(sendIndexing.size());

				sendFunc(&numOfData, sizeof(numOfData)); //1. Send number of data to be sent.

				for (const MbedTlsObj::BigNumber& index : sendIndexing)
				{
					std::array<uint8_t, DhtStates::sk_keySizeByte> keyBuf{};
					index.ToBinary(keyBuf);
					sendFunc(keyBuf.data(), keyBuf.size()); //2. Send Key of the data.

					std::vector<uint8_t> data = DeleteData(index);

					uint64_t sizeOfData = static_cast<uint64_t>(data.size());
					sendFunc(&sizeOfData, sizeof(sizeOfData)); //3. Send size of data to be sent.
					sendFunc(data.data(), data.size()); //4. Send data. - Done!
				}
			}

			/**
			* \brief	Sends migrating data to remote DHT store.
			*
			* \tparam	SendFuncT	Type of the send function t. Must have the form of "void FuncName(const void* buf, size_t size)".
			* \param	sendFunc	The send function.
			* \param	start   	The start position on the ring (INclusive).
			* \param	end			The end position on the ring (EXclusive).
			*/
			template<typename SendFuncT>
			void SendMigratingData(SendFuncT sendFunc, const MbedTlsObj::BigNumber& start, const MbedTlsObj::BigNumber& end)
			{
				SendMigratingData(sendFunc, DeleteIndexing(start, end));
			}

			/**
			* \brief	Receive migrating data from remote DHT store.
			*
			* \tparam	RecvFuncT	Type of the receive function t. Must have the form of "void FuncName(void* buf, size_t size)".
			* \param	recvFunc	The receive function.
			*/
			template<typename RecvFuncT>
			void RecvMigratingData(RecvFuncT recvFunc)
			{
				std::unique_lock<std::mutex> indexingLock(GetIndexingMutex());

				uint64_t numOfData = 0;
				recvFunc(&numOfData, sizeof(numOfData)); //1. Receive number of data to be received.

				for (uint64_t i = 0; i < numOfData; ++i)
				{
					std::array<uint8_t, DhtStates::sk_keySizeByte> keyBuf{};
					recvFunc(keyBuf.data(), keyBuf.size()); //2. Receive key of the data.
					MbedTlsObj::ConstBigNumber key(keyBuf);

					uint64_t sizeOfData = 0;
					recvFunc(&sizeOfData, sizeof(sizeOfData)); //3. Receive size of data to be sent.

					std::vector<uint8_t> data(sizeOfData);
					recvFunc(data.data(), data.size()); //4. Receive data. - Done!

					SaveData(key, std::move(data));
				}
			}

			virtual bool IsResponsibleFor(const MbedTlsObj::BigNumber& key) const;

			virtual void GetValue(const MbedTlsObj::BigNumber& key, std::vector<uint8_t>& data) override;

		protected:

			virtual std::vector<uint8_t> DeleteData(const MbedTlsObj::BigNumber& key) override;

			virtual void SaveData(const MbedTlsObj::BigNumber& key, std::vector<uint8_t>&& data) override;

		private:

		};
	}
}
