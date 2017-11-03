// DecryptClient.cpp : 定义控制台应用程序的入口点。
//

#include "ace/ACE.h"

int ACE_TMAIN (int argc, ACE_TCHAR *agrv[])
{
	if (argc != 3)
	{
		::MessageBox(NULL,"arg error","",MB_OK);
		return -1;
	}

	if (!::CopyFile(agrv[1],agrv[2],FALSE))
	{
		::MessageBox(NULL,agrv[1],"",MB_OK);
		::MessageBox(NULL,agrv[2],"",MB_OK);
		return -1;
	}
	
	return 0;
}

