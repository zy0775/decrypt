// DecryptServer.cpp : 定义控制台应用程序的入口点。
//

#include "ace/ACE.h"
#include "common/ZxCCInterface.h"
#include "IPublisher.h"
#include "MsgService.h"

int ACE_TMAIN (int argc, ACE_TCHAR *agrv[])
{
	CCommonInit::Init();

	int ret = CMsgService::Instance()->Start();
	if (ret != 0)
	{
		return 0;
	}

	while(1)
	{
		char pcmd[1024];
		memset(pcmd,0,1024);
		std::cin.getline(pcmd,1024);
		CZxCCInterface::Instance()->ExecuteCommand(pcmd);
	}
	CCommonInit::Release();
	return 0;
}

