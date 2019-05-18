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
				 * \param [out]	output	The output. Please note that the serialized data will be pushed back
				 * 							to the vector, so that any existing data in the vector will be kept at
				 * 							the front.
				 */
				virtual void Serialize(std::vector<uint8_t>& output) const = 0;

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
