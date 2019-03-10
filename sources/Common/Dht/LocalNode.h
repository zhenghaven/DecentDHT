///**
//* \file Node.h
//* \author Haofan Zheng
//* \brief Header file that declares and implements the Node class and Fingertable class.
//*
//*/
#pragma once

#include <memory>

#include "Node.h"
#include "FingerTable.h"

namespace Decent
{
	namespace Dht
	{
		template<typename IdType, size_t KeySizeByte, typename AddrType>
		class LocalNode : public Node<IdType, AddrType>, public std::enable_shared_from_this<LocalNode<IdType, KeySizeByte, AddrType> >
		{
		public: //static member:
			typedef Node<IdType, AddrType> NodeBaseType;
			typedef typename NodeBaseType::NodeBasePtrType NodeBasePtrType;
			static constexpr size_t sk_keySizeBit = KeySizeByte * BITS_PER_BYTE;
			typedef std::array<IdType, sk_keySizeBit + 1> Pow2iArrayType;


		public:

			/**
			 * \brief	Construct the Node. This will also initialize the finger table as if this node is the
			 * 			only node in the network.
			 *
			 * \param	id			  	the ID of this node.
			 * \param	addr		  	The address.
			 * \param	ringSmallestId	Smallest possible identifier on the ring.
			 * \param	ringLargestId 	Largest possible identifier on the ring.
			 * \param	pow2iArray	  	Array of 2^i, where 0 <= i <= ([key size in bits] + 1).
			 */
			LocalNode(const IdType& id, const AddrType& addr, const IdType& ringSmallestId, const IdType& ringLargestId, std::shared_ptr<const Pow2iArrayType> pow2iArray) :
				m_id(id),
				m_addr(addr),
				m_ringSmallestId(ringSmallestId),
				m_ringLargestId(ringLargestId),
				m_cirRange(m_ringSmallestId, m_ringLargestId),
				m_pow2iArray(pow2iArray),
				m_fingerTable(m_id, m_cirRange, pow2iArray)
			{}

			/** \brief	Destructor */
			virtual ~LocalNode()
			{}

			/**
			 * \brief	Join the existing network.
			 *
			 * \param [in,out]	node	the first node to contact with to initialize join process.
			 */
			virtual void Join(NodeBaseType& node)
			{
				m_fingerTable.JoinTo(node);
				GetImmediateSuccessor()->SetImmediatePredecessor(GetSelfPtr());

				UpdateOthers();

				//TODO:
				//m_fingerTable.GetImmediateSuccessor()->MigrateData(m_localKeys, m_id, m_predcId);
			}

			/**
			 * \brief	Called when this node is joining the existing network and after its finger table is
			 * 			re-initialized.
			 */
			void UpdateOthers()
			{
				for (size_t i = 0; i < sk_keySizeBit; ++i)
				{
					const IdType& pow2i = (*m_pow2iArray)[i];
					IdType pow2im1 = pow2i - 1;
					NodeBasePtrType p = this->FindPredecessor(m_cirRange.Minus(m_id, pow2im1));
					NodeBasePtrType selfPtr = GetSelfPtr();
					p->UpdateFingerTable(selfPtr, i);
				}
			}

			/**
			 * \brief	Called when a new node is joining the existing network, and this node possibly need
			 * 			to update its finger table according to that new node. This function will forward the
			 * 			call to FingerTable class, and notify its predecessor.
			 *
			 * \param [in,out]	s		   	Pointer to the new node.
			 * \param 		  	i		   	row of the finger table that needs to be checked.
			 */
			virtual void UpdateFingerTable(NodeBasePtrType& s, size_t i) override
			{
				m_fingerTable.UpdateFingerTable(s, i);
			}

			/**
			* \brief Leaving the network.
			* \param debugOutStr [out] return the character string that contains the trace of the lookup operation.
			*/
			//virtual void Leave();

			/**
			* \brief Called when this node is leaving the network.
			* \param debugOutStr [out] return the character string that contains the trace of the lookup operation.
			*/
			//void DeUpdateOthers(std::string& debugOutStr);
			
			/**
			* \brief Called when a new node is leaving the network, and this node possibly need to de-update its finger table accordingly.
			*        This function will forward the call to FingerTable class, and notify its predecessor.
			* \param oldID ID of the leaving node.
			* \param s Pointer to the successor of the leaving node.
			* \param sid ID of leaving node's successor.
			* \param i Row of the finger table that needs to be checked.
			* \param debugOutStr [out] return the character string that contains the trace of the lookup operation.
			* \return Return true if the table is de-updated, otherwise, return false.
			*/
			//virtual void DeUpdateFingerTable(NodeIdType oldId, Node* s, NodeIdType sid, size_t i, std::string& debugOutStr);
			
			/**
			 * \brief	Find the successor of a specific key value.
			 *
			 * \param	key	Key to lookup.
			 *
			 * \return	The found successor.
			 */
			virtual NodeBasePtrType FindSuccessor(const IdType& key) override
			{
				if (m_cirRange.IsWithinNC(key, m_fingerTable.GetImmediatePredecessor()->GetNodeId(), m_id))
				{
					return GetSelfPtr();
				}
				return FindPredecessor(key)->GetImmediateSuccessor();
			}

			/**
			 * \brief	Find the predcessor of a specific key value.
			 *
			 * \param	key	Key to lookup.
			 *
			 * \return	The found predecessor.
			 */
			virtual NodeBasePtrType FindPredecessor(const IdType& key) override
			{
				const IdType& immediateSucId = GetImmediateSuccessor()->GetNodeId();
				if (m_cirRange.IsWithinNC(key, m_id, immediateSucId))
				{
					return GetSelfPtr();
				}
				NodeBasePtrType nextHop = m_fingerTable.GetClosetPrecFinger(key);
				return nextHop->FindPredecessor(key);
			}

			/**
			 * \brief	Get the immediate successor.
			 *
			 * \return	The pointer to the immediate successor.
			 */
			virtual NodeBasePtrType GetImmediateSuccessor() override
			{
				NodeBasePtrType res = m_fingerTable.GetImmediateSuccessor();
				return res ? res : GetSelfPtr();
			}

			/**
			 * \brief	Get the immediate predecessor node.
			 *
			 * \return	Return the pointer to immediate predecessor node.
			 */
			virtual NodeBasePtrType GetImmediatePredecessor() override
			{
				NodeBasePtrType res = m_fingerTable.GetImmediatePredecessor();
				return res ? res : GetSelfPtr();
			}

			/**
			 * \brief	Set the immediate predecessor of this node.
			 *
			 * \param	pred	pointer to the new immediate predecessor node.
			 */
			virtual void SetImmediatePredecessor(NodeBasePtrType pred) override
			{
				if (pred->GetAddress() == m_addr && pred->GetNodeId() == m_id)
				{
					m_fingerTable.SetImmediatePredecessor(GetSelfPtr());
				}
				else
				{
					m_fingerTable.SetImmediatePredecessor(pred);
				}
			}

			/**
			 * \brief	Gets the ID of this node
			 *
			 * \return	The node's identifier.
			 */
			virtual const IdType& GetNodeId() override
			{
				return m_id;
			}

			/**
			 * \brief	Gets the address of this node
			 *
			 * \return	The address of this node.
			 */
			virtual const AddrType& GetAddress() override
			{
				return m_addr;
			}

		protected:

			/**
			 * \brief	Gets shared pointer to self
			 *
			 * \return	The self shared pointer.
			 */
			NodeBasePtrType GetSelfPtr()
			{
				return this->shared_from_this();
			}

		private:
			IdType m_id;
			AddrType m_addr;
			IdType m_ringSmallestId;
			IdType m_ringLargestId;
			CircularRange<IdType> m_cirRange; 
			std::shared_ptr<const Pow2iArrayType> m_pow2iArray;
			FingerTable<IdType, KeySizeByte, AddrType> m_fingerTable;
			//	std::map<KeyType, ValueType> m_localKeys;

		};
	}
}

//class Node 
//{
//public:
//
//
//	/**
//	* \brief Insert a new value to the network.
//	* \param key Key for that value.
//	* \param value Value to be inserted.
//	* \param debugOutStr [out] return the character string that contains the trace of the lookup operation.
//	*/
//	virtual void Insert(KeyType key, ValueType value, std::string& debugOutStr);
//
//	/**
//	* \brief Remove a value from the network.
//	* \param key Key associated to that value.
//	* \param debugOutStr [out] return the character string that contains the trace of the lookup operation.
//	*/
//	virtual void Remove(KeyType key, std::string& debugOutStr);
//
//protected:
//
//	/**
//	* \brief Find a value locally.
//	* \param key Key associated to that value.
//	*/
//	virtual ValueType FindLocally(KeyType key) const;
//
//	/**
//	* \brief Insert a value locally.
//	* \param key Key associated to that value.
//	* \param value Value to be inserted.
//	*/
//	virtual void InsertLocally(KeyType key, ValueType value);
//
//	/**
//	* \brief Remove a value locally.
//	* \param key Key associated to that value.
//	*/
//	virtual void RemoveLocally(KeyType key);
//
//
//	/**
//	* \brief Caller needs to migrate the data from this node.
//	* \param dataMap [out] The container to receive the data.
//	* \param start The starting value of the range. In counter-clockwise.
//	* \param end The ending value (not included) of the range. In counter-clockwise.
//	* \param debugOutStr [out] return the character string that shows what values have been migrated.
//	*/
//	virtual void MigrateData(std::map<KeyType, ValueType>& dataMap, const KeyType start, const KeyType end, std::string& debugOutStr);
//
//	/**
//	* \brief Caller needs to migrate the data to this node.
//	* \param dataMap [in, out] The container to give the data. It will be emptied afterward.
//	* \param debugOutStr [out] return the character string that shows what values have been migrated.
//	*/
//	virtual void DeMigrateData(std::map<KeyType, ValueType>& dataMap, std::string& debugOutStr);
//};
//
