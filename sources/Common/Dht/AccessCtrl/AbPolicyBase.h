#pragma once

#include <memory>
#include <vector>

#include "AbAttributeList.h"

namespace Decent
{
	namespace Dht
	{
		namespace AccessCtrl
		{
			class AbPolicyBase
			{
			public:
				AbPolicyBase() = default;
				
				virtual ~AbPolicyBase()
				{}

				virtual bool Examine(const AbAttributeList& attrList) const = 0;

				/**
				 * \brief	Gets serialized size, so that you can reserve the size of the buffer beforehand.
				 *
				 * \return	The serialized size.
				 */
				virtual size_t GetSerializedSize() const = 0;

				/**
				* \brief	Serialize this object to the given stream
				*
				* \param [out]	destIt	The iterator point to the begin of the binary block to insert the data.
				* \param 	   	end   	The end of the binary block.
				*
				* \return	A std::vector&lt;uint8_t&gt;::iterator, which points to the end of inserted data.
				*/
				virtual std::vector<uint8_t>::iterator Serialize(std::vector<uint8_t>::iterator destIt, std::vector<uint8_t>::iterator end) const = 0;

				/**
				 * \brief	Gets related attributes
				 *
				 * \param [out]	outputList	List of related attributes.
				 */
				virtual void GetRelatedAttributes(AbAttributeList& outputList) const = 0;

				/**
				 * \brief	'and' operator
				 *
				 * \param	rhs	The right hand side.
				 *
				 * \return	The result of the operation.
				 */
				//virtual std::unique_ptr<AbPolicyBase> operator&(const std::unique_ptr<AbPolicyBase>& rhs) const = 0;

				/**
				 * \brief	'or' operator
				 *
				 * \param	rhs	The right hand side.
				 *
				 * \return	The result of the operation.
				 */
				//virtual std::unique_ptr<AbPolicyBase> operator|(const std::unique_ptr<AbPolicyBase>& rhs) const = 0;
			};
		}
	}
}
