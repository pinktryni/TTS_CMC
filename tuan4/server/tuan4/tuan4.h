
// tuan4.h : main header file for the PROJECT_NAME application
//

#pragma once

#ifndef __AFXWIN_H__
	#error "include 'pch.h' before including this file for PCH"
#endif

#include "resource.h"		// main symbols


// Ctuan4App:
// See tuan4.cpp for the implementation of this class
//

class Ctuan4App : public CWinApp
{
public:
	Ctuan4App();

// Overrides
public:
	virtual BOOL InitInstance();

// Implementation

	DECLARE_MESSAGE_MAP()
};

extern Ctuan4App theApp;
