#pragma once

#include "AbPolicyBase.h"

namespace Decent
{
	namespace Dht
	{
		namespace AccessCtrl
		{
			class AbPolicyUnaryOp : public AbPolicyBase
			{
			public: //Static members:
				typedef uint8_t FlagType;

			public:
				AbPolicyUnaryOp() = delete;

				AbPolicyUnaryOp(std::unique_ptr<AbPolicyBase>&& var);

				AbPolicyUnaryOp(const AbPolicyUnaryOp& rhs) = delete;

				AbPolicyUnaryOp(AbPolicyUnaryOp&& rhs);

				virtual ~AbPolicyUnaryOp();

				virtual bool Examine(const AbAttributeList& attrList) const = 0;

				virtual size_t GetSerializedSize() const override;

				virtual uint8_t GetFlagByte() const = 0;

				/**
				* \brief	Serialize this object to the given stream
				*
				* \param [out]	destIt	The iterator point to the begin of the binary block to insert the data.
				* \param 	   	end   	The end of the binary block.
				*
				* \return	A std::vector&lt;uint8_t&gt;::iterator, which points to the end of inserted data.
				*/
				virtual std::vector<uint8_t>::iterator Serialize(std::vector<uint8_t>::iterator destIt, std::vector<uint8_t>::iterator end) const override;

				virtual void GetRelatedAttributes(AbAttributeList& outputList) const override;

			protected:
				std::unique_ptr<AbPolicyBase> m_var;
			};
		}
	}
}
