// DecryptClientMFC.cpp : 定义应用程序的类行为。
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


// CDecryptClientMFCApp 构造

CDecryptClientMFCApp::CDecryptClientMFCApp()
{
	// TODO: 在此处添加构造代码，
	// 将所有重要的初始化放置在 InitInstance 中
}


// 唯一的一个 CDecryptClientMFCApp 对象

CDecryptClientMFCApp theApp;


// CDecryptClientMFCApp 初始化

BOOL CDecryptClientMFCApp::InitInstance()
{
	CCommonInit::Init();

	CWinApp::InitInstance();

	// 标准初始化
	// 如果未使用这些功能并希望减小
	// 最终可执行文件的大小，则应移除下列
	// 不需要的特定初始化例程
	// 更改用于存储设置的注册表项
	// TODO: 应适当修改该字符串，
	// 例如修改为公司或组织名
	SetRegistryKey(_T("应用程序向导生成的本地应用程序"));

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
		// TODO: 在此处放置处理何时用“确定”来关闭
		//  对话框的代码

	}
	else if (nResponse == IDCANCEL)
	{
		// TODO: 在此放置处理何时用“取消”来关闭
		//  对话框的代码
	}

	CCommonInit::Release();

	// 由于对话框已关闭，所以将返回 FALSE 以便退出应用程序，
	//  而不是启动应用程序的消息泵。
	return FALSE;
}
