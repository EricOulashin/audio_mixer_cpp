#include "stdafx.h"
#include "CCustomEditListBox.h"
#include "GUITools.h"

void CCustomEditListBox::OnBrowse()
{
	int nSel = GetSelItem(); // This needs to be done first in order to be correct
	const CString filenameFilter = _T("Wave|*.wav||");
	CString selectedFilename;
	if (letUserBrowseForOneFile(filenameFilter, selectedFilename, this) == IDOK)
	{
		if (nSel == GetCount()) // New item
		{
			nSel = AddItem(selectedFilename);
			SelectItem(nSel);
		}
		else
			SetItemText(nSel, selectedFilename);
	}
}