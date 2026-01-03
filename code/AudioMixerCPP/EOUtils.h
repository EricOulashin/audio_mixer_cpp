#ifndef __EO_UTILS_H__
#define __EO_UTILS_H__

namespace EOUtils
{
	bool isBigEndian();
	static bool machineIsBigEndian = isBigEndian();
	void reverseBytes(void *start, int size);
	size_t getFileSize(const char* pFilename);
}

#endif
