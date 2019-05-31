#pragma once

#include <memory>
#include <vector>

namespace Decent
{
	namespace Dht
	{
		namespace AccessCtrl
		{
			class EntityList;
			class EntityItem;

			class EntityBasedControl
			{
			public: //static member:

				/**
				 * \brief	Construct an entity based control that allows all accesses
				 *
				 * \return	An EntityBasedControl.
				 */
				static EntityBasedControl AllowAll();

				/**
				 * \brief	Construct an entity based control that denies all accesses
				 *
				 * \return	An EntityBasedControl.
				 */
				static EntityBasedControl DenyAll();

			public:
				EntityBasedControl() = delete;

				/**
				 * \brief	Constructor
				 *
				 * \param	r	A std::unique_ptr&lt;AbPolicyBase&gt;, read policy. An empty pointer represents
				 * 				the "allow all" policy.
				 * \param	w	A std::unique_ptr&lt;AbPolicyBase&gt;, write policy. An empty pointer represents
				 * 				the "allow all" policy.
				 * \param	x	A std::unique_ptr&lt;AbPolicyBase&gt;, execute policy. An empty pointer
				 * 				represents the "allow all" policy.
				 */
				EntityBasedControl(std::unique_ptr<EntityList> r, std::unique_ptr<EntityList> w, std::unique_ptr<EntityList> x);

				/**
				 * \brief	Constructor for parsing policy stored in binary. Note: Parsing in here is not simply
				 * 			parse till the end, instead, it will stop when enough data is parsed.
				 *
				 * \param	it 	The iterator.
				 * \param	end	The end.
				 */
				EntityBasedControl(std::vector<uint8_t>::const_iterator& it, std::vector<uint8_t>::const_iterator end);

				/**
				 * \brief	Move constructor
				 *
				 * \param [in,out]	rhs	The right hand side.
				 */
				EntityBasedControl(EntityBasedControl&& rhs);

				/** \brief	Destructor */
				virtual ~EntityBasedControl();

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
				 * \param	item	The item.
				 *
				 * \return	True if it succeeds, false if it fails.
				 */
				bool ExamineRead(const EntityItem& item) const;

				/**
				 * \brief	Examine write policy
				 *
				 * \param	item	The item.
				 *
				 * \return	True if it succeeds, false if it fails.
				 */
				bool ExamineWrite(const EntityItem& item) const;

				/**
				 * \brief	Examine execute policy
				 *
				 * \param	item	The item.
				 *
				 * \return	True if it succeeds, false if it fails.
				 */
				bool ExamineExecute(const EntityItem& item) const;

			private:
				/** \brief	The read policy */
				std::unique_ptr<EntityList> m_rPolicy;

				/** \brief	The write policy */
				std::unique_ptr<EntityList> m_wPolicy;

				/** \brief	The execute policy */
				std::unique_ptr<EntityList> m_xPolicy;
			};
		}
	}
}
