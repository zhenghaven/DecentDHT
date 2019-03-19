///**
//* \file Node.h
//* \author Haofan Zheng
//* \brief Header file that declares and implements the Node class and Fingertable class.
//*
//*/
#pragma once

#include <memory>

#include "NodeBase.h"
#include "FingerTable.h"

namespace Decent
{
	namespace Dht
	{
		template<typename IdType, size_t KeySizeByte, typename AddrType>
		class LocalNode : public NodeBase<IdType, AddrType>, public std::enable_shared_from_this<LocalNode<IdType, KeySizeByte, AddrType> >
		{
		public: //static member:
			typedef NodeBase<IdType, AddrType> NodeBaseType;
			typedef typename NodeBaseType::NodeBasePtr NodeBasePtr;
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
					NodeBasePtr p = this->FindPredecessor(m_cirRange.Minus(m_id, pow2im1));
					NodeBasePtr selfPtr = GetSelfPtr();
					p->UpdateFingerTable(selfPtr, static_cast<uint64_t>(i));
				}
			}

			/**
			 * \brief	Called when a new node is joining the existing network, and this node possibly need
			 * 			to update its finger table according to that new node. This function will forward the
			 * 			call to FingerTable class, and notify its predecessor.
			 *
			 * \param [in,out]	succ	Pointer to the new node.
			 * \param 		  	i   	row of the finger table that needs to be checked.
			 */
			virtual void UpdateFingerTable(NodeBasePtr& succ, uint64_t i) override
			{
				if (m_fingerTable.UpdateFingerTable(succ, static_cast<size_t>(i)))
				{
					GetImmediatePredecessor()->UpdateFingerTable(succ, i);
				}
			}

			/** \brief	Leaving the network. */
			virtual void Leave()
			{
				GetImmediateSuccessor()->SetImmediatePredecessor(GetImmediatePredecessor());

				DeUpdateOthers();
			}

			/** \brief	Called when this node is leaving the network. */
			void DeUpdateOthers()
			{
				NodeBasePtr succ = GetImmediateSuccessor();

				for (size_t i = 0; i < sk_keySizeBit; ++i)
				{
					const IdType& pow2i = (*m_pow2iArray)[i];
					IdType pow2im1 = pow2i - 1;
					NodeBasePtr p = this->FindPredecessor(m_cirRange.Minus(m_id, pow2im1));
					if (p->GetNodeId() != this->GetNodeId()) //Do not update the node self.
					{
						p->DeUpdateFingerTable(this->GetNodeId(), succ, static_cast<uint64_t>(i));
					}
				}
			}

			/**
			 * \brief	Called when a new node is leaving the network, and this node possibly need to de-
			 * 			update its finger table accordingly. This function will forward the call to
			 * 			FingerTable class, and notify its predecessor.
			 *
			 * \param 		  	oldId	ID of the leaving node.
			 * \param [in,out]	succ 	Pointer to the successor of the leaving node.
			 * \param 		  	i	 	Row of the finger table that needs to be checked.
			 */
			virtual void DeUpdateFingerTable(const IdType& oldId, NodeBasePtr& succ, uint64_t i) override
			{
				if (m_fingerTable.DeUpdateFingerTable(oldId, succ, static_cast<size_t>(i)) && oldId != GetImmediatePredecessor()->GetNodeId())
				{
					GetImmediatePredecessor()->DeUpdateFingerTable(oldId, succ, i);
				}
			}
			
			/**
			 * \brief	Find the successor of a specific key value.
			 *
			 * \param	key	Key to lookup.
			 *
			 * \return	The found successor.
			 */
			virtual NodeBasePtr FindSuccessor(const IdType& key) override
			{
				if (this->IsResponsibleFor(key))
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
			virtual NodeBasePtr FindPredecessor(const IdType& key) override
			{
				const IdType& immediateSucId = GetImmediateSuccessor()->GetNodeId();
				if (m_cirRange.IsWithinNC(key, m_id, immediateSucId))
				{
					return GetSelfPtr();
				}
				NodeBasePtr nextHop = m_fingerTable.GetClosetPrecFinger(key);
				return nextHop->FindPredecessor(key);
			}

			/**
			 * \brief	Get the immediate successor.
			 *
			 * \return	The pointer to the immediate successor.
			 */
			virtual NodeBasePtr GetImmediateSuccessor() override
			{
				NodeBasePtr res = m_fingerTable.GetImmediateSuccessor();
				return res ? res : GetSelfPtr();
			}

			/**
			 * \brief	Get the immediate predecessor node.
			 *
			 * \return	Return the pointer to immediate predecessor node.
			 */
			virtual NodeBasePtr GetImmediatePredecessor() override
			{
				NodeBasePtr res = m_fingerTable.GetImmediatePredecessor();
				return res ? res : GetSelfPtr();
			}

			/**
			 * \brief	Set the immediate predecessor of this node.
			 *
			 * \param	pred	pointer to the new immediate predecessor node.
			 */
			virtual void SetImmediatePredecessor(NodeBasePtr pred) override
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
			virtual const AddrType& GetAddress() const override
			{
				return m_addr;
			}

			bool IsResponsibleFor(const IdType& key)
			{
				return m_cirRange.IsWithinNC(key, GetImmediatePredecessor()->GetNodeId(), m_id);
			}

		protected:

			/**
			 * \brief	Gets shared pointer to self
			 *
			 * \return	The self shared pointer.
			 */
			NodeBasePtr GetSelfPtr()
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
		};
	}
}
