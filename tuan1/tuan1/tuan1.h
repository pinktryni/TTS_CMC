
// tuan1.h : main header file for the PROJECT_NAME application
//

#pragma once

#ifndef __AFXWIN_H__
	#error "include 'pch.h' before including this file for PCH"
#endif

#include "resource.h"		// main symbols


// Ctuan1App:
// See tuan1.cpp for the implementation of this class
//

class Ctuan1App : public CWinApp
{
public:
	Ctuan1App();

// Overrides
public:
	virtual BOOL InitInstance();

// Implementation

	DECLARE_MESSAGE_MAP()
};

extern Ctuan1App theApp;
