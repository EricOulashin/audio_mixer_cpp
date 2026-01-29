#ifndef __CCUSTOM_EDIT_LIST_BOX_H__
#define __CCUSTOM_EDIT_LIST_BOX_H__

// This is a customized CVSListBox to customize the behavior of
// the "browse" button (labeled "...") to allow the user to
// browse for & choose a file
// https://burnburn.acmerich.com/2014/10/how-to-use-cvslistbox-customize-browse-button/


#include "afxvslistbox.h"

class CCustomEditListBox : public CVSListBox
{
	private:
		void OnBrowse() override;
};

#endif