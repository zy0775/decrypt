// DecryptClientMFC.h : PROJECT_NAME Ӧ�ó������ͷ�ļ�
//

#pragma once

#ifndef __AFXWIN_H__
	#error "�ڰ������ļ�֮ǰ������stdafx.h�������� PCH �ļ�"
#endif

#include "resource.h"		// ������


// CDecryptClientMFCApp:
// �йش����ʵ�֣������ DecryptClientMFC.cpp
//

class CDecryptClientMFCApp : public CWinApp
{
public:
	CDecryptClientMFCApp();

// ��д
	public:
	virtual BOOL InitInstance();

// ʵ��

	DECLARE_MESSAGE_MAP()
};

extern CDecryptClientMFCApp theApp;