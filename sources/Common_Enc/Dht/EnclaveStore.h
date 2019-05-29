#pragma once

#include "../../Common/Dht/StoreBase.h"

#include <DecentApi/Common/MbedTls/BigNumber.h>

#include "DhtStates.h"

namespace Decent
{
	namespace Dht
	{
		class EnclaveStore : public StoreBase<MbedTlsObj::BigNumber, uint64_t>
		{
		public:
			EnclaveStore(const MbedTlsObj::BigNumber& ringStart, const MbedTlsObj::BigNumber& ringEnd);

			virtual ~EnclaveStore();

			virtual bool IsResponsibleFor(const MbedTlsObj::BigNumber& key) const;

		protected:
			virtual std::vector<uint8_t> SaveDataFile(const MbedTlsObj::BigNumber& key, const std::vector<uint8_t>& meta, const std::vector<uint8_t>& data) override;

			virtual void DeleteDataFile(const MbedTlsObj::BigNumber& key) override;

			virtual std::vector<uint8_t> ReadDataFile(const MbedTlsObj::BigNumber& key, const std::vector<uint8_t>& tag, std::vector<uint8_t>& meta) override;

			virtual std::vector<uint8_t> MigrateOneDataFile(const MbedTlsObj::BigNumber& key, const std::vector<uint8_t>& tag, std::vector<uint8_t>& meta) override;

		private:
			void* m_memStore;
		};
	}
}
