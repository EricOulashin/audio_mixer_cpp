#include "stdafx.h"
#include "GUITools.h"
#include "afxdlgs.h"

INT_PTR letUserBrowseForOneFile(const CString& pFileFilter, CString& pSelectedFile, CWnd* pParentWnd)
{
	pSelectedFile = _T("");

	CFileDialog fd(TRUE, _T(""), _T(""), OFN_HIDEREADONLY, pFileFilter, pParentWnd);

	fd.m_ofn.Flags |= OFN_ENABLETEMPLATE;
	fd.m_ofn.hInstance = AfxGetInstanceHandle();
	fd.m_ofn.lpTemplateName = _T("DIALOG_PREVIEW");

	INT_PTR statusCode = fd.DoModal();
	if (statusCode == IDOK)
		pSelectedFile = fd.GetPathName();
	return statusCode;
}