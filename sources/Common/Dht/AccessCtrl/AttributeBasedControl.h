#pragma once

#include <memory>
#include <vector>

namespace Decent
{
	namespace Dht
	{
		namespace AccessCtrl
		{
			class AbPolicyBase;
			class AbAttributeList;

			class AttributeBasedControl
			{
			public: //static member:

				/**
				 * \brief	Construct an entity based control that allows all accesses
				 *
				 * \return	An AttributeBasedControl.
				 */
				static AttributeBasedControl AllowAll();

				/**
				 * \brief	Construct an entity based control that denies all accesses
				 *
				 * \return	An AttributeBasedControl.
				 */
				static AttributeBasedControl DenyAll();

			public:
				AttributeBasedControl() = delete;

				/**
				 * \brief	Constructor
				 *
				 * \param	r	A std::unique_ptr&lt;AbPolicyBase&gt;, read policy.
				 * \param	w	A std::unique_ptr&lt;AbPolicyBase&gt;, write policy.
				 * \param	x	A std::unique_ptr&lt;AbPolicyBase&gt;, execute policy.
				 */
				AttributeBasedControl(std::unique_ptr<AbPolicyBase> r, std::unique_ptr<AbPolicyBase> w, std::unique_ptr<AbPolicyBase> x);

				/**
				 * \brief	Constructor for parsing policy stored in binary. Note: Parsing in here is not simply
				 * 			parse till the end, instead, it will stop when enough data is parsed.
				 *
				 * \param	it 	The iterator.
				 * \param	end	The end.
				 */
				AttributeBasedControl(std::vector<uint8_t>::const_iterator& it, std::vector<uint8_t>::const_iterator end);

				/**
				 * \brief	Move constructor
				 *
				 * \param [in,out]	rhs	The right hand side.
				 */
				AttributeBasedControl(AttributeBasedControl&& rhs);

				/** \brief	Destructor */
				virtual ~AttributeBasedControl();

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
				 * \brief	Examine read policy
				 *
				 * \param	attrList	List of attributes.
				 *
				 * \return	True if it succeeds, false if it fails.
				 */
				bool ExamineRead(const AbAttributeList& attrList) const;

				/**
				 * \brief	Examine write policy
				 *
				 * \param	attrList	List of attributes.
				 *
				 * \return	True if it succeeds, false if it fails.
				 */
				bool ExamineWrite(const AbAttributeList& attrList) const;

				/**
				 * \brief	Examine execute policy
				 *
				 * \param	attrList	List of attributes.
				 *
				 * \return	True if it succeeds, false if it fails.
				 */
				bool ExamineExecute(const AbAttributeList& attrList) const;

				/**
				 * \brief	Gets read related attribute list
				 *
				 * \param [in,out]	attrList	List of attributes.
				 */
				void GetReadRelatedAttributes(AbAttributeList& attrList) const;

				/**
				 * \brief	Gets write related attribute list
				 *
				 * \param [in,out]	attrList	List of attributes.
				 */
				void GetWriteRelatedAttributes(AbAttributeList& attrList) const;

				/**
				 * \brief	Gets execute related attribute list
				 *
				 * \param [in,out]	attrList	List of attributes.
				 */
				void GetExecuteRelatedAttributes(AbAttributeList& attrList) const;

			private:
				/** \brief	The read policy */
				std::unique_ptr<AbPolicyBase> m_rPolicy;

				/** \brief	The write policy */
				std::unique_ptr<AbPolicyBase> m_wPolicy;

				/** \brief	The execute policy */
				std::unique_ptr<AbPolicyBase> m_xPolicy;
			};
		}
	}
}
