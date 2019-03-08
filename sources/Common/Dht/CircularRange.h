/**
* \file CircularRange.h
* \author Haofan Zheng
* \brief Header file that declares and implements the CircularRange class.
*
*/
#pragma once

#include <cstdint>

#include <stdexcept>

/**
* \class CircularRange
* \brief A clockwise circular range.
*/
template<typename T, T circleStart, T circleEnd>
class CircularRange
{
private:

	/**
	 * \brief	A helper function to check if a value is within a range.
	 */
	static constexpr bool Range(const T v, const T start, const T end)
	{
		return start <= v && v <= end;
	}

	/**
	* \brief A helper function to check if the testing range is inside the circle.
	*/
	static constexpr bool CheckTestingRangeNN(const T start, const T end)
	{
		return (circleStart <= start && circleStart <= end) && (start <= circleEnd && end <= circleEnd);
	}

	/**
	* \brief A helper function to determine the inclusion of a circular range.
	*/
	static constexpr bool CircularRangeNN(const T v, const T start, const T end)
	{
		return Range(v, start, circleEnd) || Range(v, circleStart, end);
	}

	/**
	* \brief A helper function to perform the minus operation.
	*/
	static constexpr T MinusChecked(const T a, const T b)
	{
		return a >= b ? a - b : (circleEnd - b + 1 + a);
	}

public:
	CircularRange() = delete;
	CircularRange(const CircularRange&) = delete;
	CircularRange(CircularRange&&) = delete;

	/**
	* \brief Check if value v is on the circle.
	*/
	static constexpr bool IsOnCircle(const T v)
	{
		return Range(v, circleStart, circleEnd);
	}

	/**
	* \brief Is within the range of (start, end).
	* \param v Value to be tested.
	* \param start Start value of the range.
	* \param end End value of the range.
	* \param comCircleSEE Is it a complete circle when start equals to end?
	*/
	static constexpr bool IsWithinNN(const T v, const T start, const T end, bool comCircleSEE = true)
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
				(start < end ? Range(v, start, end) : //CASE 2: A simple range
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
	static constexpr bool IsWithinNC(const T v, const T start, const T end, bool comCircleSEE = true)
	{
		return (IsWithinNN(v, start, end, comCircleSEE) || (v == end)) ;
	}

	/**
	* \brief Is within the range of [start, end).
	* \param v Value to be tested.
	* \param start Start value of the range.
	* \param end End value of the range.
	* \param comCircleSEE Is it a complete circle when start equals to end?
	*/
	static constexpr bool IsWithinCN(const T v, const T start, const T end, bool comCircleSEE = true)
	{
		return (IsWithinNN(v, start, end, comCircleSEE) || (v == start)) ;
	}

	/**
	* \brief Is not within the range of (start, end].
	* \param v Value to be tested.
	* \param start Start value of the range.
	* \param end End value of the range.
	* \param comCircleSEE Is it a complete circle when start equals to end?
	*/
	static constexpr bool IsNotWithinNC(const T v, const T start, const T end, bool comCircleSEE = true)
	{
		return !IsWithinNC(v, start, end, comCircleSEE);
	}

	/**
	* \brief Calculate (aOncirc - b) on a circular range.
	* \param aOnCirc An initial value that is on the circle and needs to be deducted.
	* \param b Value to be deducted from aOnCirc.
	*/
	static constexpr T Minus(const T aOnCirc, const uint64_t b)
	{
		return IsOnCircle(aOnCirc) ? MinusChecked(aOnCirc, b % (circleEnd - circleStart + 1)) : (throw std::logic_error("The Value of a is not on the circle!"));
	}
};
