// FontRotate.h : main header file for the FONTROTATE application
//

#if !defined(AFX_FONTROTATE_H__774FC8AC_53F9_4F14_85C7_D6FEA03984DC__INCLUDED_)
#define AFX_FONTROTATE_H__774FC8AC_53F9_4F14_85C7_D6FEA03984DC__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// main symbols

/////////////////////////////////////////////////////////////////////////////
// CFontRotateApp:
// See FontRotate.cpp for the implementation of this class
//

class CFontRotateApp : public CWinApp
{
public:
	CFontRotateApp();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CFontRotateApp)
	public:
	virtual BOOL InitInstance();
	//}}AFX_VIRTUAL

// Implementation

	//{{AFX_MSG(CFontRotateApp)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_FONTROTATE_H__774FC8AC_53F9_4F14_85C7_D6FEA03984DC__INCLUDED_)
