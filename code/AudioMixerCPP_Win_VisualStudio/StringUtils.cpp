#include "StringUtils.h"

using std::string;

namespace EOUtils
{
	string CStringToStdString(const CString& pCStr)
	{
		CT2CA pszConvertedAnsiString(pCStr);
		return string(pszConvertedAnsiString);
	}

	CString stdStringToCString(const string& pStr)
	{
		return CString(pStr.c_str());
	}
}