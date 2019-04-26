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
			EnclaveStore(const MbedTlsObj::BigNumber& ringStart, const MbedTlsObj::BigNumber& ringEnd) :
				StoreBase(ringStart, ringEnd)
			{}

			virtual ~EnclaveStore();

			void MigrateFrom(const uint64_t& addr, const MbedTlsObj::BigNumber& start, const MbedTlsObj::BigNumber& end) override;

			virtual void MigrateTo(const uint64_t& addr) override;

			virtual bool IsResponsibleFor(const MbedTlsObj::BigNumber& key) const;

			virtual void GetValue(const MbedTlsObj::BigNumber& key, std::vector<uint8_t>& data) override;

		protected:
			virtual void DeleteDataFile(const std::string& keyStr) override;

			virtual void SaveDataFile(const std::string& keyStr, const std::vector<uint8_t>& data) override;

		private:

		};
	}
}
