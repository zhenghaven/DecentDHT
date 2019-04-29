#pragma once

#include <string>
#include <vector>
#include <memory>
#include <map>
#include <mutex>

namespace Decent
{
	namespace Dht
	{
		class MemKeyValueStore
		{
		public: //Static members:
			typedef std::string KeyType;
			typedef std::pair<size_t, std::unique_ptr<uint8_t[]> > ValueType;
			typedef std::map<KeyType, ValueType> MapType;
			typedef std::pair<KeyType, ValueType> KeyValPair;

		public:
			/** \brief	Default constructor */
			MemKeyValueStore();

			/** \brief	Destructor */
			virtual ~MemKeyValueStore();

			/**
			 * \brief	Stores a key value pair. Existing pair will be overwritten if exist.
			 *
			 * \param 	  	key	The key.
			 * \param [in]	val	The value, whose ownership will be moved in.
			 */
			virtual void Store(const KeyType& key, ValueType&& val);

			/**
			 * \brief	Reads the value associated with given key
			 *
			 * \param	key	The key to read.
			 *
			 * \return	A copy of the value.
			 */
			virtual ValueType Read(const KeyType& key);

			/**
			 * \brief	Deletes the value associated with given key
			 *
			 * \param	key	The key to delete.
			 *
			 * \return	The value that moved out.
			 */
			virtual ValueType Delete(const KeyType& key);

			/**
			 * \brief	Migrates a range of key value pairs. 
			 *
			 * \param	lowerVal 	The lower key value.
			 * \param	higherVal	The higher key value.
			 *
			 * \return	A std::vector&lt;KeyValPair&gt; which is a list of key-value pairs that has been moved out.
			 */
			virtual std::vector<KeyValPair> Migrate(const KeyType& lowerVal, const KeyType& higherVal);

			/**
			 * \brief	Migrate all values in this key-value store.
			 *
			 * \return	A std::vector&lt;KeyValPair&gt; which is a list of key-value pairs that has been moved out.
			 */
			virtual std::vector<KeyValPair> MigrateAll();

		protected:

			/**
			 * \brief	Delete a item from the map. NOTE: Protected function; assume 'it' is not pointing to
			 * 			the end; assume map has been locked.
			 *
			 * \param	it	The Iterator to delete.
			 *
			 * \return	A ValueType.
			 */
			virtual ValueType Delete(MapType::iterator it);

		private:
			std::mutex m_mapMutex;
			MapType m_map;
		};
	}
}
