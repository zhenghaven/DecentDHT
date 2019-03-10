#pragma once

#include <memory>
#include <cstdint>

namespace Decent
{
	namespace Dht
	{
		template<typename IdType, typename AddrType>
		class Node
		{
		public: //static members:
			typedef std::shared_ptr<Node> NodeBasePtrType;

		public:
			/** \brief	Default constructor */
			Node() = default;

			/** \brief	Destructor */
			virtual ~Node()
			{}

			/**
			 * \brief	Find the successor of a specific key value.
			 *
			 * \param	key	Key to lookup.
			 *
			 * \return	The found successor.
			 */
			virtual NodeBasePtrType FindSuccessor(const IdType& key) = 0;

			/**
			 * \brief	Find the predcessor of a specific key value.
			 *
			 * \param	key	Key to lookup.
			 *
			 * \return	The found predecessor.
			 */
			virtual NodeBasePtrType FindPredecessor(const IdType& key) = 0;

			/**
			 * \brief	Get the immediate successor.
			 *
			 * \return	The pointer to the immediate successor.
			 */
			virtual NodeBasePtrType GetImmediateSuccessor() = 0;

			/**
			 * \brief	Get the immediate predecessor node.
			 *
			 * \return	Return the pointer to immediate predecessor node.
			 */
			virtual NodeBasePtrType GetImmediatePredecessor() = 0;

			/**
			 * \brief	Set the immediate predecessor of this node.
			 *
			 * \param	pred	pointer to the new immediate predecessor node.
			 */
			virtual void SetImmediatePredecessor(NodeBasePtrType pred) = 0;

			/**
			 * \brief	Called when a new node is joining the existing network, and this node possibly need
			 * 			to update its finger table according to that new node. This function will forward the
			 * 			call to FingerTable class, and notify its predecessor.
			 *
			 * \param [in,out]	succ	Pointer to the new node.
			 * \param 		  	i   	row of the finger table that needs to be checked.
			 */
			virtual void UpdateFingerTable(NodeBasePtrType& s, uint64_t i) = 0;

			/**
			 * \brief	Called when a new node is leaving the network, and this node possibly need to de-
			 * 			update its finger table accordingly. This function will forward the call to
			 * 			FingerTable class, and notify its predecessor.
			 *
			 * \param 		  	oldId	ID of the leaving node.
			 * \param [in,out]	succ 	Pointer to the successor of the leaving node.
			 * \param 		  	i	 	Row of the finger table that needs to be checked.
			 */
			virtual void DeUpdateFingerTable(const IdType& oldId, NodeBasePtrType& succ, uint64_t i) = 0;

			/**
			 * \brief	Gets the ID of this node
			 *
			 * \return	The node's identifier.
			 */
			virtual const IdType& GetNodeId() = 0;

			/**
			 * \brief	Gets the address of this node
			 *
			 * \return	The address of this node.
			 */
			virtual const AddrType& GetAddress() = 0;

		};
	}
}
