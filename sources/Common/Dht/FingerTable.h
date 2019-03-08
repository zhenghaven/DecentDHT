//
// Created by Aaron Chu on 3/7/19.
//
#pragma once

#include <memory>
#include <string>
#include <vector>

#include "Node.h"

namespace Decent
{
	namespace Dht
	{
		/**
		* \brief Defines each row of the finger table.
		*/
		template<typename ConstIdType>
		struct FingerTableRecord
		{
			ConstIdType m_startId;
			ConstIdType m_endId;
			std::unique_ptr<Node<ConstIdType> > m_nodeCnt;

			template<typename IdType>
			FingerTableRecord(const IdType& start, const IdType& end) :
				m_startId(start),
				m_endId(end),
				m_nodeCnt()
			{}

			~FingerTableRecord() {}
		};

		class FingerTable
		{
		public:
			/**
			 * \brief Construct the FingerTable. This will also initialize the finger table as if the owner is the only node in the network.
			 * \param ownerNodeId the id of node hosting the finger table.
			 * \param ownerNode the pointer to the node hosting the finger table.
			 */
			FingerTable(NodeIdType ownerNodeId, Node* ownerNode);

			/**
			* \brief Destructor.
			*/
			~FingerTable();

		//	/**
		//	* \brief Called when this node is joining the existing network. The finger table will be re-initialized according to the existing node.
		//	* \param exNode Pointer to an existing node.
		//	* \param outPred [out] return the immediate predecessor.
		//	* \param outSucc [out] return the immediate successor.
		//	* \param debugOutStr [out] return the character string that contains the trace of the lookup operation.
		//	*/
		//	void JoinTo(Node* exNode, Node*& outPred, Node*& outSucc, std::string& debugOutStr);

		//	/**
		//	* \brief Called when this node is joining the existing network and after its finger table is re-initialized.
		//	* \param debugOutStr [out] return the character string that contains the trace of the lookup operation.
		//	*/
		//	void UpdateOthers(std::string& debugOutStr);

		//	/**
		//	* \brief Called when this node is leaving the network.
		//	* \param debugOutStr [out] return the character string that contains the trace of the lookup operation.
		//	*/
		//	void DeUpdateOthers(std::string& debugOutStr);

		//	/**
		//	* \brief Called when a new node is joining the existing network, and this node possibly need to update its finger table according to that new node.
		//	* \param s Pointer to the new node.
		//	* \param sid ID of the new node.
		//	* \param i row of the finger table that needs to be checked.
		//	* \param debugOutStr [out] return the character string that contains the trace of the lookup operation.
		//	* \return Return true if the table is updated, otherwise, return false.
		//	*/
		//	bool UpdateFingerTable(Node* s, NodeIdType sid, size_t i, std::string& debugOutStr);

		//	/**
		//	* \brief Called when a new node is leaving the network, and this node possibly need to de-update its finger table accordingly.
		//	* \param oldID ID of the leaving node.
		//	* \param s Pointer to the successor of the leaving node.
		//	* \param sid ID of leaving node's successor.
		//	* \param i Row of the finger table that needs to be checked.
		//	* \param debugOutStr [out] return the character string that contains the trace of the lookup operation.
		//	* \return Return true if the table is de-updated, otherwise, return false.
		//	*/
		//	bool DeUpdateFingerTable(NodeIdType oldId, Node* s, NodeIdType sid, size_t i, std::string& debugOutStr);

		//	/**
		//	* \brief Get the closet predcessor for a specific key.
		//	* \param id Key for loopup.
		//	* \return The pointer to the closet predcessor node.
		//	*/
		//	Node* GetClosetPrecFinger(KeyType id) const;

		//	/**
		//	* \brief Get the immediate successor.
		//	* \return The pointer to the immediate successor.
		//	*/
		//	Node* GetImmediateSuccessor() const;

		//	/**
		//	* \brief Get the ID of immediate successor.
		//	* \return The ID of immediate successor.
		//	*/
		//	NodeIdType GetImmediateSuccessorId() const;

		//	/**
		//	* \brief Get the styled character string that shows the finger table.
		//	* \return The styled character string that shows the finger table.
		//	*/
		//	std::string ToStyledString() const;

		//	/**
		//	* \brief Print the styled character string to a specific output stream.
		//	* \param stream Output stream.
		//	*/
		//	void PrintStyledString(std::basic_ostream<char, std::char_traits<char> >& stream) const;

		private:
			NodeIdType m_nodeId;
			Node* m_node;
			std::vector<FingerTableRecord> m_fingerTable;
		};

	}
}


