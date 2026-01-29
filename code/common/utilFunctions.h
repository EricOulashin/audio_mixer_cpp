#ifndef __UTIL_FUNCTIONS_H__
#define __UTIL_FUNCTIONS_H__


namespace EOUtils
{
	// Returns the highest positive value of a numeric type.
	template<typename T> T maxValue()
	{
		T value = 0;

		while (value <= 0)
			--value;

		return(value);
	}

	// Returns the lowest value of a numeric type.
	template<typename T> T minValue()
	{
		T value = 1;

		while (value > 0)
			++value;

		return(value);
	}
}

#endif