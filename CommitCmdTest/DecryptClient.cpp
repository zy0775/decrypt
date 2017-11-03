#include "ace/ACE.h"
#include "Data.h"
#include "IPublisher.h"
#include "ZxLog.h"
#include "common/ZxCCInterface.h"

typedef struct EnumFunArg
{
	HWND hWnd;  
	DWORD dwProcessId;     
}EnumFunArg,*LPEnumFunArg;

BOOL CALLBACK EnumFunc(HWND hwnd,LPARAM lParam)  
{
	EnumFunArg* pArg = (LPEnumFunArg)lParam;     
	DWORD processId;
	GetWindowThreadProcessId(hwnd,&processId);
	if(processId == pArg->dwProcessId)    
	{     
		pArg->hWnd = hwnd;
		char temp[256] = {0};
		::GetWindowText(hwnd,temp,256);
		//LOG_INFO(hwnd << "---" << temp);
		if (CData(temp).find(" - Commit - TortoiseSVN") != CDATA_NPOS)
		{
			LOG_INFO(hwnd << "---" << temp);
			return FALSE;
		} 
	}  
	return TRUE;     
}

HWND ReturnWnd(DWORD processID)
{  
	HWND retWnd = NULL;
	EnumFunArg wi;
	wi.dwProcessId = processID;
	wi.hWnd = NULL;
	EnumWindows(EnumFunc,(LPARAM)&wi);
	if(wi.hWnd)
	{
		retWnd = wi.hWnd;
	}
	return retWnd;
}  


int ACE_TMAIN (int argc, ACE_TCHAR *agrv[])
{
	CCommonInit::Init();
	ReturnWnd(0);
	while(1)
	{
		char pcmd[1024];
		memset(pcmd,0,1024);
		std::cin.getline(pcmd,1024);
		int pid = CData(pcmd).convertInt();
		HWND hwnd = ReturnWnd(pid);
		LOG_INFO("---pid---" << pid << "---wnd---" << hwnd);
	}
	CCommonInit::Release();
	return 0;
}

