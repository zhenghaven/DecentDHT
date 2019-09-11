#pragma once

#include <string>
#include <vector>

namespace Decent
{
	namespace Dht
	{
		class UntrustedKeyValueStore
		{
		public:
			UntrustedKeyValueStore();

			virtual ~UntrustedKeyValueStore();

			virtual void Save(const std::string& key, const std::vector<uint8_t>& value);

			virtual std::vector<uint8_t> Read(const std::string& key);

			virtual void Delete(const std::string& key);

			virtual std::vector<uint8_t> MigrateOne(const std::string& key);

		private:
			void* m_ptr;
		};
	}
}
