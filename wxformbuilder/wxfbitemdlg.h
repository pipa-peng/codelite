//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//
// copyright            : (C) 2008 by Eran Ifrah                            
// file name            : wxfbitemdlg.h              
//                                                                          
// -------------------------------------------------------------------------
// A                                                                        
//              _____           _      _     _ _                            
//             /  __ \         | |    | |   (_) |                           
//             | /  \/ ___   __| | ___| |    _| |_ ___                      
//             | |    / _ \ / _  |/ _ \ |   | | __/ _ )                     
//             | \__/\ (_) | (_| |  __/ |___| | ||  __/                     
//              \____/\___/ \__,_|\___\_____/_|\__\___|                     
//                                                                          
//                                                  F i l e                 
//                                                                          
//    This program is free software; you can redistribute it and/or modify  
//    it under the terms of the GNU General Public License as published by  
//    the Free Software Foundation; either version 2 of the License, or     
//    (at your option) any later version.                                   
//                                                                          
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

#ifndef __wxfbitemdlg__
#define __wxfbitemdlg__

/**
@file
Subclass of wxFBItemBaseDlg, which is generated by wxFormBuilder.
*/

#include "wxfbitembasedlg.h"
class IManager;

enum wxFBItemKind {
	wxFBItemKind_Unknown,
	wxFBItemKind_Dialog,
	wxFBItemKind_Dialog_With_Buttons,
	wxFBItemKind_Frame,
	wxFBItemKind_Panel
};

struct wxFBItemInfo {
	wxString className;
	wxString virtualFolder;
	wxString title;
	wxString file;
	wxFBItemKind kind;
};

/** Implementing wxFBItemBaseDlg */
class wxFBItemDlg : public wxFBItemBaseDlg
{
	IManager *m_mgr;
	
protected:
	// Handlers for wxFBItemBaseDlg events.
	void OnGenerate( wxCommandEvent& event );
	void OnGenerateUI( wxUpdateUIEvent& event );
	void OnCancel( wxCommandEvent& event );
	void OnBrowseVD(wxCommandEvent &event);
public:
	/** Constructor */
	wxFBItemDlg( wxWindow* parent, IManager *mgr );
	wxFBItemInfo GetData();
	void DisableTitleField();
};

#endif // __wxfbitemdlg__
