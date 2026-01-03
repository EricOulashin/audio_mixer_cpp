#ifndef __GUI_TOOLS_H__
#define __GUI_TOOLS_H__

#include <atlstr.h>

/**
 * @brief Lets the user select a single file.
 * @param[in] pFileFilter The filename filter
 * @param[out] pSelectedFile This will be populated with the selected filename, if the user selected one
 * @param[in] pParentWnd A pointer to the parent window.  Defaults to nullptr.
 * @return The status code from the file dialog
 **/
INT_PTR letUserBrowseForOneFile(const CString& pFileFilter, CString& pSelectedFile, CWnd* pParentWnd = nullptr);

#endif