#pragma once

#include <vector>

#include "EntityBasedControl.h"
#include "AttributeBasedControl.h"

namespace Decent
{
	namespace Dht
	{
		namespace AccessCtrl
		{
			class FullPolicy
			{
			public:
				FullPolicy() = delete;

				/**
				 * \brief	Constructor for parsing policy stored in binary. Note: Parsing in here is not simply
				 * 			parse till the end, instead, it will stop when enough data is parsed.
				 *
				 * \param	it 	The iterator.
				 * \param	end	The end.
				 */
				FullPolicy(std::vector<uint8_t>::const_iterator& it, std::vector<uint8_t>::const_iterator end);

				/**
				 * \brief	Constructor
				 *
				 * \param [in]	enclavePolicy	The enclave policy.
				 * \param [in]	clientPolicy 	The client policy.
				 */
				FullPolicy(EntityBasedControl&& enclavePolicy, AttributeBasedControl&& clientPolicy);

				/** \brief	Destructor */
				virtual ~FullPolicy();

				/**
				* \brief	Gets serialized size
				*
				* \return	The serialized size.
				*/
				virtual size_t GetSerializedSize() const;

				/**
				* \brief	Serialize this object to the given stream
				*
				* \param [out]	destIt	The iterator point to the begin of the binary block to insert the data.
				* \param 	   	end   	The end of the binary block.
				*
				* \return	A std::vector&lt;uint8_t&gt;::iterator, which points to the end of inserted data.
				*/
				virtual std::vector<uint8_t>::iterator Serialize(std::vector<uint8_t>::iterator destIt, std::vector<uint8_t>::iterator end) const;

				/**
				 * \brief	Gets the access control policy for enclaves
				 *
				 * \return	The enclave policy.
				 */
				EntityBasedControl& GetEnclavePolicy();

				/**
				 * \brief	Gets the access control policy for enclaves
				 *
				 * \return	The enclave policy.
				 */
				const EntityBasedControl& GetEnclavePolicy() const;

				/**
				 * \brief	Gets the access control policy for client
				 *
				 * \return	The client policy.
				 */
				AttributeBasedControl& GetClientPolicy();

				/**
				 * \brief	Gets the access control policy for client
				 *
				 * \return	The client policy.
				 */
				const AttributeBasedControl& GetClientPolicy() const;

			private:
				EntityBasedControl m_enclavePolicy;
				AttributeBasedControl m_clientPolicy;
			};
		}
	}
}
