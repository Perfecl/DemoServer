#pragma once

#ifndef __AFXWIN_H__
#error "�ڰ������ļ�֮ǰ������stdafx.h�������� PCH �ļ�"
#endif

#include "resource.h"

class CServerConsoleApp : public CWinApp
{
public:
	CServerConsoleApp();

	virtual BOOL InitInstance();

	DECLARE_MESSAGE_MAP()
};

extern CServerConsoleApp theApp;