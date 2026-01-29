#ifndef __STRING_UTILS_H__
#define __STRING_UTILS_H__

#include "stdafx.h"
#include <string>

namespace EOUtils
{
	std::string CStringToStdString(const CString& pCStr);

	CString stdStringToCString(const std::string& pStr);
}

#endif