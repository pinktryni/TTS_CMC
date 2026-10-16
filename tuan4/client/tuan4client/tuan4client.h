
// tuan4client.h : main header file for the PROJECT_NAME application
//

#pragma once

#ifndef __AFXWIN_H__
	#error "include 'pch.h' before including this file for PCH"
#endif

#include "resource.h"		// main symbols


// Ctuan4clientApp:
// See tuan4client.cpp for the implementation of this class
//

class Ctuan4clientApp : public CWinApp
{
public:
	Ctuan4clientApp();

// Overrides
public:
	virtual BOOL InitInstance();

// Implementation

	DECLARE_MESSAGE_MAP()
};

extern Ctuan4clientApp theApp;
