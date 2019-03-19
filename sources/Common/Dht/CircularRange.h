/**
* \file CircularRange.h
* \author Haofan Zheng
* \brief Header file that declares and implements the CircularRange class.
*
*/
#pragma once

#include <cstdint>

#include <stdexcept>

namespace Decent
{
	namespace Dht
	{
		/**
		* \class CircularRange
		* \brief A clockwise circular range.
		*/
		template<typename ValType>
		class CircularRange
		{
		private:
			const ValType& m_circleStart;
			const ValType& m_circleEnd;

			/**
			 * \brief	A helper function to check if a value is within the range of [start, end].
			 */
			static constexpr bool RangeCC(const ValType& v, const ValType& start, const ValType& end)
			{
				return start <= v && v <= end;
			}

			/**
			 * \brief	A helper function to check if a value is within the range of (start, end).
			 */
			static constexpr bool RangeNN(const ValType& v, const ValType& start, const ValType& end)
			{
				return start < v && v < end;
			}

			/**
			 * \brief	A helper function to check if a value is within the range of [start, end).
			 */
			static constexpr bool RangeCN(const ValType& v, const ValType& start, const ValType& end)
			{
				return start <= v && v < end;
			}

			/**
			 * \brief	A helper function to check if a value is within the range of (start, end].
			 */
			static constexpr bool RangeNC(const ValType& v, const ValType& start, const ValType& end)
			{
				return start < v && v <= end;
			}

			/**
			* \brief A helper function to check if the testing range is inside the circle.
			*/
			bool CheckTestingRangeNN(const ValType& start, const ValType& end) const
			{
				return (m_circleStart <= start && m_circleStart <= end) && (start <= m_circleEnd && end <= m_circleEnd);
			}

			/**
			* \brief A helper function to determine the inclusion of a circular range.
			*/
			bool CircularRangeNN(const ValType& v, const ValType& start, const ValType& end) const
			{
				return RangeNC(v, start, m_circleEnd) || RangeCN(v, m_circleStart, end);
			}

			/**
			* \brief A helper function to perform the minus operation.
			*/
			ValType MinusChecked(const ValType& a, const ValType& b) const
			{
				return a >= b ? a - b : (m_circleEnd - b + 1 + a);
			}

		public:
			CircularRange() = delete;
			CircularRange(const CircularRange&) = delete;
			CircularRange(CircularRange&&) = delete;

			CircularRange(const ValType& circleStart, const ValType& circleEnd) :
				m_circleStart(circleStart),
				m_circleEnd(circleEnd)
			{}

			/**
			* \brief Check if value v is on the circle.
			*/
			bool IsOnCircle(const ValType& v) const
			{
				return RangeCC(v, m_circleStart, m_circleEnd);
			}

			/**
			* \brief Is within the range of (start, end).
			* \param v Value to be tested.
			* \param start Start value of the range.
			* \param end End value of the range.
			* \param comCircleSEE Is it a complete circle when start equals to end?
			*/
			bool IsWithinNN(const ValType& v, const ValType& start, const ValType& end, bool comCircleSEE = true) const
			{
				/*
					if (start == end)
					{//A complete circle:
						return v != start;
					}
					else if (start < end)
					{//A simple case range:
						return SimpleRangeNN(v, start, end);
					}
					else
					{//A special circular range:
						return (m_isEndClose && v == m_end) ||
							(m_isStartClose && v == m_start) ||
							SimpleRangeNN(v, start, m_end) ||
							SimpleRangeNN(v, m_start, end);
					}
				*/
				//Invalid case: 'start' or 'end' is out of circle range, or v is not on the circle.
				return (!CheckTestingRangeNN(start, end) || !IsOnCircle(v)) ? (throw std::logic_error("Testing range is outside of the circle!")) : //Make sure the testing range is valid.
					(start == end ? comCircleSEE && (v != start) : //CASE 1: A complete circle
					(start < end ? RangeNN(v, start, end) : //CASE 2: A simple range
						(CircularRangeNN(v, start, end)) //CASE 3: A circular range
						));
			}

			/**
			* \brief Is within the range of (start, end].
			* \param v Value to be tested.
			* \param start Start value of the range.
			* \param end End value of the range.
			* \param comCircleSEE Is it a complete circle when start equals to end?
			*/
			bool IsWithinNC(const ValType& v, const ValType& start, const ValType& end, bool comCircleSEE = true) const
			{
				return (IsWithinNN(v, start, end, comCircleSEE) || (v == end));
			}

			/**
			* \brief Is within the range of [start, end).
			* \param v Value to be tested.
			* \param start Start value of the range.
			* \param end End value of the range.
			* \param comCircleSEE Is it a complete circle when start equals to end?
			*/
			bool IsWithinCN(const ValType& v, const ValType& start, const ValType& end, bool comCircleSEE = true) const
			{
				return (IsWithinNN(v, start, end, comCircleSEE) || (v == start));
			}

			/**
			* \brief Is not within the range of (start, end].
			* \param v Value to be tested.
			* \param start Start value of the range.
			* \param end End value of the range.
			* \param comCircleSEE Is it a complete circle when start equals to end?
			*/
			bool IsNotWithinNC(const ValType& v, const ValType& start, const ValType& end, bool comCircleSEE = true) const
			{
				return !IsWithinNC(v, start, end, comCircleSEE);
			}

			/**
			* \brief Calculate (aOncirc - b) on a circular range.
			* \param aOnCirc An initial value that is on the circle and needs to be deducted.
			* \param b Value to be deducted from aOnCirc.
			*/
			ValType Minus(const ValType& aOnCirc, const ValType& b) const
			{
				return IsOnCircle(aOnCirc) ? MinusChecked(aOnCirc, b % ((m_circleEnd - m_circleStart) + 1)) : (throw std::logic_error("The Value of a is not on the circle!"));
			}
		};
	}
}



