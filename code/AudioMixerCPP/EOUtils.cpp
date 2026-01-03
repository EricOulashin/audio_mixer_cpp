#include "EOUtils.h"
#include <algorithm>
#include <fstream>
using std::reverse;

namespace EOUtils
{
	bool isBigEndian()
	{
		union {
			unsigned int i;
			char c[4];
		} bint = { 0x01020304 };

		return bint.c[0] == 1;
	}

	void reverseBytes(void *pStart, int pSize)
	{
		char *istart = (char*)pStart, *iend = (char*)(istart + pSize);
		reverse(istart, iend);
	}

	size_t getFileSize(const char* pFilename)
	{
		std::ifstream::pos_type fileSize = 0;
		std::ifstream in(pFilename, std::ifstream::ate | std::ifstream::binary);
		if (in.is_open())
		{
			fileSize = in.tellg();
			in.close();
		}
		return (size_t)fileSize;
	}
}