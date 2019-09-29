#pragma once

#include <memory>
#include <string>
#include <vector>

#include <DecentApi/Common/Common.h>

#include "NodeBase.h"
#include "CircularRange.h"

namespace Decent
{
	namespace Dht
	{
		/**
		 * \brief	Number of bits per byte.
		 */
		constexpr size_t BITS_PER_BYTE = 8;

		/**
		* \brief Defines each row of the finger table.
		*/
		template<typename IdType, typename AddrType>
		struct FingerTableRecord
		{
			typedef NodeBase<IdType, AddrType> NodeBaseType;
			typedef typename NodeBaseType::NodeBasePtr NodeBasePtr;

			IdType m_startId;
			IdType m_endId;
			NodeBasePtr m_node;

			FingerTableRecord() = delete;

			FingerTableRecord(const IdType& start, const IdType& end) :
				m_startId(start),
				m_endId(end),
				m_node()
			{}

			FingerTableRecord(FingerTableRecord&& rhs) :
				m_startId(std::forward<IdType>(rhs.m_startId)),
				m_endId(std::forward<IdType>(rhs.m_endId)),
				m_node(std::move(rhs.m_node))
			{}

			~FingerTableRecord() {}
		};

		template<typename IdType, size_t KeySizeByte, typename AddrType, bool checkCircleRange>
		class FingerTable
		{
		public: //Static members:
			typedef NodeBase<IdType, AddrType> NodeBaseType;
			typedef typename NodeBaseType::NodeBasePtr NodeBasePtr;
			static constexpr size_t sk_keySizeBit = KeySizeByte * BITS_PER_BYTE;
			typedef std::array<IdType, sk_keySizeBit + 1> Pow2iArrayType;

		public:

			/**
			 * \brief	Construct the FingerTable. This will also initialize the finger table as if the owner
			 * 			is the only node in the network.
			 *
			 * \exception	std::runtime_error	Raised when a runtime error condition occurs.
			 *
			 * \param	ownerNodeId	the id of node hosting the finger table.
			 * \param	circleRange	Circular range tester object.
			 * \param	pow2iArray 	Array of 2^i, where 0 <= i <= ([key size in bits] + 1).
			 */
			FingerTable(const IdType& ownerNodeId, const CircularRange<IdType, checkCircleRange>& circleRange, std::shared_ptr<const Pow2iArrayType> pow2iArray) :
				m_nodeId(ownerNodeId),
				m_cirRange(circleRange),
				m_tableRecords()
			{
				if (!m_cirRange.IsOnCircle(m_nodeId))
				{
				    throw std::runtime_error("Node ID used to contruct finger table is out of range!");
				}
				
				const IdType& pow2m = (*pow2iArray)[KeySizeByte * BITS_PER_BYTE];
				
				IdType prevEndId = (m_nodeId + static_cast<int64_t>(1)) % pow2m;
				for (size_t i = 0; i < KeySizeByte * BITS_PER_BYTE; ++i)
				{
					IdType nextEndId = (m_nodeId + (*pow2iArray)[i + 1]) % pow2m;
					m_tableRecords.push_back(FingerTableRecord<IdType, AddrType>(prevEndId, nextEndId));
					prevEndId = std::move(nextEndId);
				}
			}

			/**
			* \brief Destructor.
			*/
			virtual ~FingerTable()
			{}

			/**
			 * \brief	Get the closet predcessor for a specific key.
			 *
			 * \param	id	Key for loopup.
			 *
			 * \return	The pointer to the closet predcessor node.
			 */
			NodeBasePtr GetClosetPrecFinger(const IdType& id) const
			{
				for (auto rit = m_tableRecords.crbegin(); rit != m_tableRecords.crend(); ++rit)
				{
					const NodeBasePtr& node = rit->m_node;
					const IdType& nodeId = node ? node->GetNodeId() : m_nodeId;

					if (m_cirRange.IsWithinNN(nodeId, m_nodeId, id))
					{
						return node;
					}
				}
				return nullptr;
			}

			/**
			 * \brief	Get the immediate successor.
			 *
			 * \return	The pointer to the immediate successor.
			 */
			NodeBasePtr GetImmediateSuccessor() const
			{
				return m_tableRecords[0].m_node;
			}

			/**
			* \brief	Get the immediate predecessor node.
			*
			* \return	Return the pointer to immediate predecessor node.
			*/
			NodeBasePtr GetImmediatePredecessor() const
			{
				return m_predecessor;
			}

			/**
			* \brief	Set the immediate predecessor of this node.
			*
			* \param	pred	pointer to the new immediate predecessor node.
			*/
			void SetImmediatePredecessor(NodeBasePtr pred)
			{
				m_predecessor = pred;
			}

			/**
			 * \brief	Called when this node is joining the existing network. The finger table will be re-
			 * 			initialized according to the existing node.
			 *
			 * \param [in,out]	exNode	Pointer to an existing node.
			 */
			void JoinTo(NodeBaseType& exNode)
			{
				m_tableRecords[0].m_node = exNode.FindSuccessor(m_tableRecords[0].m_startId);
				m_predecessor = m_tableRecords[0].m_node->GetImmediatePredecessor();
				for (size_t i = 0; i < (m_tableRecords.size() - 1); ++i)
				{
					if (m_cirRange.IsWithinCN(m_tableRecords[i + 1].m_startId, m_nodeId, m_tableRecords[i].m_node->GetNodeId()))
					{
						m_tableRecords[i + 1].m_node = m_tableRecords[i].m_node;
					}
					else
					{
						m_tableRecords[i + 1].m_node = exNode.FindSuccessor(m_tableRecords[i + 1].m_startId);
					}
				}
			}

			/**
			 * \brief	Called when a new node is joining the existing network, and this node possibly need
			 * 			to update its finger table according to that new node.
			 *
			 * \param [in,out]	succ	Pointer to the new node.
			 * \param 		  	i   	row of the finger table that needs to be checked.
			 *
			 * \return	Return true if the table is updated, otherwise, return false.
			 */
			bool UpdateFingerTable(NodeBasePtr& succ, size_t i)
			{
				const IdType& nodeIthId = m_tableRecords[i].m_node ? m_tableRecords[i].m_node->GetNodeId() : m_nodeId;
				if (m_cirRange.IsWithinCN(succ->GetNodeId(), m_tableRecords[i].m_startId, nodeIthId, false))
				{
					m_tableRecords[i].m_node = succ;
					PrintTable();
					return true;
				}
				return false;
			}

			/**
			 * \brief	Called when a new node is leaving the network, and this node possibly need to de-
			 * 			update its finger table accordingly.
			 *
			 * \param 		  	oldId	ID of the leaving node.
			 * \param [in,out]	succ 	Pointer to the successor of the leaving node.
			 * \param 		  	i	 	Row of the finger table that needs to be checked.
			 *
			 * \return	Return true if the table is de-updated, otherwise, return false.
			 */
			bool DeUpdateFingerTable(const IdType& oldId, NodeBasePtr& succ, size_t i)
			{
				const IdType& nodeIthId = m_tableRecords[i].m_node ? m_tableRecords[i].m_node->GetNodeId() : m_nodeId;
				if (nodeIthId == oldId)
				{
					m_tableRecords[i].m_node = succ;
					PrintTable();
					return true;
				}
				return false;
			}

			void PrintTable() const
			{
				//for (size_t i = 0; i < KeySizeByte * BITS_PER_BYTE; ++i)
				//{
				//	LOGI("%s | %s ", m_tableRecords[i].m_startId.ToBigEndianHexStr().c_str(), 
				//		m_tableRecords[i].m_node ? m_tableRecords[i].m_node->GetNodeId().ToBigEndianHexStr().c_str() : m_nodeId.ToBigEndianHexStr().c_str());
				//}
			}

		private:
			const IdType& m_nodeId;
			const CircularRange<IdType, checkCircleRange>& m_cirRange;
			std::vector<FingerTableRecord<IdType, AddrType> > m_tableRecords;
			NodeBasePtr m_predecessor;
		};

	}
}


