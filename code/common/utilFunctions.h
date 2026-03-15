#ifndef __UTIL_FUNCTIONS_H__
#define __UTIL_FUNCTIONS_H__

#include <limits>

namespace EOUtils
{
	// Returns the highest positive value of a numeric type.
	template<typename T> T maxValue()
	{
		return std::numeric_limits<T>::max();
	}

	// Returns the lowest value of a numeric type.
	template<typename T> T minValue()
	{
		return std::numeric_limits<T>::min();
	}
}

#endif