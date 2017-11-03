// DecryptClientMFC.cpp : ����Ӧ�ó��������Ϊ��
//

#include "stdafx.h"
#include "DecryptClientMFC.h"
#include "DecryptClientMFCDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#include "ace/Process_Mutex.h"
#include "ace/Date_Time.h"
#include "IPublisher.h"
#include "ZxLog.h"

#pragma data_seg("ShareMem")
long g_last_process_start_time = 0;
#pragma data_seg()
#pragma comment(linker,"/SECTION:ShareMem,RWS")

// CDecryptClientMFCApp

BEGIN_MESSAGE_MAP(CDecryptClientMFCApp, CWinApp)
	ON_COMMAND(ID_HELP, &CWinApp::OnHelp)
END_MESSAGE_MAP()


// CDecryptClientMFCApp ����

CDecryptClientMFCApp::CDecryptClientMFCApp()
{
	// TODO: �ڴ˴���ӹ�����룬
	// ��������Ҫ�ĳ�ʼ�������� InitInstance ��
}


// Ψһ��һ�� CDecryptClientMFCApp ����

CDecryptClientMFCApp theApp;


// CDecryptClientMFCApp ��ʼ��

BOOL CDecryptClientMFCApp::InitInstance()
{
	CCommonInit::Init();

	CWinApp::InitInstance();

	// ��׼��ʼ��
	// ���δʹ����Щ���ܲ�ϣ����С
	// ���տ�ִ���ļ��Ĵ�С����Ӧ�Ƴ�����
	// ����Ҫ���ض���ʼ������
	// �������ڴ洢���õ�ע�����
	// TODO: Ӧ�ʵ��޸ĸ��ַ�����
	// �����޸�Ϊ��˾����֯��
	SetRegistryKey(_T("Ӧ�ó��������ɵı���Ӧ�ó���"));

	if (__argc != 2)
	{
		LOG_ERROR("command line parameter error,argc:" << __argc);
		return -1;
	}

	ACE_Process_Mutex* lock = new ACE_Process_Mutex("DecryptClientMFC");
	if (lock)
	{
		int result = lock->tryacquire();
		if (result == -1)
		{
			long start_time = ACE_OS::gettimeofday().msec();
			if (start_time-g_last_process_start_time>500)
			{
				return FALSE;
			}
			else
			{
				g_last_process_start_time = ACE_OS::gettimeofday().msec();
			}
			HWND hwnd = NULL;
			for (int i = 0;i<10;i++)
			{
				hwnd = FindWindow(NULL,"*DecryptClientMFC*");
				if (hwnd)
				{
					COPYDATASTRUCT cds;
					cds.dwData = 1;
					cds.lpData = __argv[1];
					cds.cbData = strlen(__argv[1]);
					LRESULT ret = ::SendMessage(hwnd,WM_COPYDATA,
						(WPARAM)GetCurrentProcessId(),(LPARAM)(LPVOID)&cds);
					return FALSE;
				}
				else
				{
					::Sleep(100);
				}
			}
		}
		else
		{
			g_last_process_start_time = ACE_OS::gettimeofday().msec();
		}
	}
	else
	{
		return FALSE;
	}

	CDecryptClientMFCDlg dlg;
	m_pMainWnd = &dlg;
	INT_PTR nResponse = dlg.DoModal();
	if (nResponse == IDOK)
	{
		// TODO: �ڴ˴����ô����ʱ�á�ȷ�������ر�
		//  �Ի���Ĵ���

	}
	else if (nResponse == IDCANCEL)
	{
		// TODO: �ڴ˷��ô����ʱ�á�ȡ�������ر�
		//  �Ի���Ĵ���
	}

	CCommonInit::Release();

	// ���ڶԻ����ѹرգ����Խ����� FALSE �Ա��˳�Ӧ�ó���
	//  ����������Ӧ�ó������Ϣ�á�
	return FALSE;
}
