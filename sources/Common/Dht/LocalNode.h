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


		public:
			LocalNode(const IdType& id, const AddrType& addr, const IdType& ringSmallestId, const IdType& ringLargestId, const std::array<std::unique_ptr<IdType>, KeySizeByte * BITS_PER_BYTE + 1>& pow2iArray) :
				m_id(id),
				m_addr(addr),
				m_ringSmallestId(ringSmallestId),
				m_ringLargestId(ringLargestId),
				m_cirRange(m_ringSmallestId, m_ringLargestId),
				m_fingerTable(m_id, m_cirRange, pow2iArray),
				m_predId(id)
			{}

			virtual ~LocalNode()
			{}

			/**
			* \brief Join the existing network.
			* \param node the first node to contact with to initialize join process.
			*/
			//virtual void Join(Node& node);

			/**
			 * \brief	Find the successor of a specific key value.
			 *
			 * \param	key	Key to lookup.
			 *
			 * \return	The found successor.
			 */
			virtual NodeBasePtrType FindSuccessor(const IdType& key) override
			{
				if (m_cirRange.IsWithinNC(key, m_predId, m_id))
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

			virtual const IdType& GetNodeId() override
			{
				return m_id;
			}

			virtual const AddrType& GetAddress() override
			{
				return m_addr;
			}

		protected:
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
			FingerTable<IdType, KeySizeByte, AddrType> m_fingerTable;
			IdType m_predId;
			//	Node* m_predecessor;
			//	std::map<KeyType, ValueType> m_localKeys;

		};
	}
}

//class Node 
//{
//public:
//	/**
//	* \brief Construct the Node. This will also initialize the finger table as if this node is the only node in the network.
//	* \param id the ID of this node.
//	*/
//	Node(const NodeIdType id);
//
//	/**
//	* \brief Destructor.
//	*/
//	virtual ~Node();
//	

//
//	/**
//	* \brief Leaving the network.
//	* \param debugOutStr [out] return the character string that contains the trace of the lookup operation.
//	*/
//	virtual void Leave(std::string& debugOutStr);
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

//

//	virtual const Node* FindSuccessor(KeyType key, std::string& debugOutStr) const;
//
//	/**
//	* \brief Find the predcessor of a specific key value.
//	* \param key Key value.
//	* \param debugOutStr [out] return the character string that contains the trace of the lookup operation.
//	*/
//	virtual Node* FindPredecessor(KeyType key, std::string& debugOutStr);
//	virtual const Node* FindPredecessor(KeyType key, std::string& debugOutStr) const;
//
//	/**
//	* \brief Called when a new node is joining the existing network, and this node possibly need to update its finger table according to that new node. 
//	*        This function will forward the call to FingerTable class, and notify its predecessor.
//	* \param s Pointer to the new node.
//	* \param sid ID of the new node.
//	* \param i row of the finger table that needs to be checked.
//	* \param debugOutStr [out] return the character string that contains the trace of the lookup operation.
//	* \return Return true if the table is updated, otherwise, return false.
//	*/
//	virtual void UpdateFingerTable(Node* s, NodeIdType sid, size_t i, std::string& debugOutStr);
//
//	/**
//	* \brief Called when a new node is leaving the network, and this node possibly need to de-update its finger table accordingly.
//	*        This function will forward the call to FingerTable class, and notify its predecessor.
//	* \param oldID ID of the leaving node.
//	* \param s Pointer to the successor of the leaving node.
//	* \param sid ID of leaving node's successor.
//	* \param i Row of the finger table that needs to be checked.
//	* \param debugOutStr [out] return the character string that contains the trace of the lookup operation.
//	* \return Return true if the table is de-updated, otherwise, return false.
//	*/
//	virtual void DeUpdateFingerTable(NodeIdType oldId, Node* s, NodeIdType sid, size_t i, std::string& debugOutStr);
//
//	/**
//	* \brief Get predecessor node.
//	* \return Return the pointer to predecessor node.
//	*/
//	Node* GetPredecessor() const;
//
//	/**
//	* \brief Get ID of this node.
//	* \return Return the ID of this node.
//	*/
//	NodeIdType GetId() const;
//
//	/**
//	* \brief Get the styled character string that shows the node's info includig the finger table.
//	* \return The styled character string that shows the node's info includig the finger table.
//	*/
//	std::string ToStyledString() const;
//
//	/**
//	* \brief Print the styled character string to a specific output stream.
//	* \param stream Output stream.
//	*/
//	void PrintStyledString(std::basic_ostream<char, std::char_traits<char> >& stream) const;
//
//protected:
//	/**
//	* \brief Get the immediate successor.
//	* \return The pointer to the immediate successor.
//	*/
//	virtual Node* GetImmediateSuccessor();
//	virtual const Node* GetImmediateSuccessor() const;
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
//	/**
//	* \brief Set the predecessor of this node.
//	* \param node pointer to the predecessor node.
//	* \param id ID of the predecessor node.
//	*/
//	virtual void SetPredecessor(Node* node, NodeIdType id);
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
//
//	/**
//	* \brief Get the styled character string that shows the local data map.
//	* \return The styled character string that shows the local data map.
//	*/
//	std::string GetDataMapStyledString() const;
//
//private:
//	NodeIdType m_id;
//	FingerTable m_fingerTable;
//	std::map<KeyType, ValueType> m_localKeys;
//	NodeIdType m_predcId;
//	Node* m_predecessor;
//};
//
