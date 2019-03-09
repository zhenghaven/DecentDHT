#pragma once

#include <memory>
#include <string>
#include <vector>

#include <DecentApi/Common/Common.h>

#include "Node.h"
#include "CircularRange.h"

namespace Decent
{
	namespace Dht
	{
		//template<typename ConstIdType>
		//class Node;

		constexpr size_t BITS_PER_BYTE = 8;

		/**
		* \brief Defines each row of the finger table.
		*/
		template<typename IdType, typename AddrType>
		struct FingerTableRecord
		{
			IdType m_startId;
			IdType m_endId;
			std::unique_ptr<Node<IdType, AddrType> > m_nodeCnt;

			FingerTableRecord() = delete;

			FingerTableRecord(const IdType& start, const IdType& end) :
				m_startId(start),
				m_endId(end),
				m_nodeCnt()
			{}

			FingerTableRecord(FingerTableRecord&& rhs) :
				m_startId(std::forward<IdType>(rhs.m_startId)),
				m_endId(std::forward<IdType>(rhs.m_endId)),
				m_nodeCnt(std::move(rhs.m_nodeCnt))
			{}

			~FingerTableRecord() {}
		};

		template<typename IdType, size_t KeySizeByte, typename AddrType>
		class FingerTable
		{
		public: //Static members:


		public:
			/**
			 * \brief Construct the FingerTable. This will also initialize the finger table as if the owner is the only node in the network.
			 * \param ownerNodeId the id of node hosting the finger table.
			 * \param ownerNode the pointer to the node hosting the finger table.
			 */
			FingerTable(const IdType& ownerNodeId, const CircularRange<const IdType, IdType>& circleRange, const std::array<std::unique_ptr<IdType>, KeySizeByte * BITS_PER_BYTE + 1>& pow2iArray) :
				m_nodeId(ownerNodeId),
				m_cirRange(circleRange),
				m_tableRecords()
			{
				if (!m_cirRange.IsOnCircle(m_nodeId))
				{
				    throw std::runtime_error("Node ID used to contruct finger table is out of range!");
				}
				
				const IdType& pow2m = *pow2iArray[KeySizeByte * BITS_PER_BYTE];
				
				IdType prevEndId = (m_nodeId + static_cast<int64_t>(1)) % pow2m;
				for (size_t i = 0; i < KeySizeByte * BITS_PER_BYTE; ++i)
				{
					IdType nextEndId = (m_nodeId + *pow2iArray[i + 1]) % pow2m;
					m_tableRecords.push_back(FingerTableRecord<IdType, AddrType>(prevEndId, nextEndId));
					prevEndId = std::move(nextEndId);
				}

				//for (size_t i = 0; i < KeySizeByte * BITS_PER_BYTE; ++i)
				//{
				//	LOGI("%s | %s ", m_tableRecords[i].m_startId.ToBigEndianHexStr().c_str(), m_tableRecords[i].m_endId.ToBigEndianHexStr().c_str());
				//}
			}

			/**
			* \brief Destructor.
			*/
			virtual ~FingerTable()
			{}

			/**
			* \brief Called when this node is joining the existing network. The finger table will be re-initialized according to the existing node.
			* \param exNode Pointer to an existing node.
			* \param outPred [out] return the immediate predecessor.
			* \param outSucc [out] return the immediate successor.
			*/
			//void JoinTo(Node<IdType>* exNode, Node*& outPred, Node*& outSucc);



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
			const IdType& m_nodeId;
			const CircularRange<const IdType, IdType>& m_cirRange;
			std::vector<FingerTableRecord<IdType, AddrType> > m_tableRecords;
		};

	}
}


