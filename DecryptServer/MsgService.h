#pragma once

#include "IPublisher.h"
#include <map>

typedef struct EnumFunArg
{
	HWND hWnd;  
	DWORD dwProcessId;     
}EnumFunArg,*LPEnumFunArg;

class CMsgService : public INetReader
{
	CMsgService(void);
public:
	virtual ~CMsgService(void);

	static CMsgService* Instance();
	static void CloseSingleton();

	virtual int OnNetMsg(const CADDRINFO& AddrInfo,CNetMsg* pMsg);
	virtual int OnConnect(const CADDRINFO& AddrInfo);
	virtual int OnAccept(const CADDRINFO& AddrInfo);
	virtual int OnClose(const CADDRINFO& AddrInfo);

	int Start();
	int UploadToSvn(CNetMsg* msg);

	CData GenGUID();
	static int ExecSvnCmd(CData cmd,int timeoutsec);
	static BOOL CALLBACK EnumFunc(HWND hwnd,LPARAM lParam);
	static HWND ReturnWnd(DWORD processID);
private:
	static CMsgService* m_instance;

	int m_listen_port;
	int m_resource_count;
	int m_ftp_port;
	CData m_ftp_user;
	CData m_ftp_password;
	CData m_svn_url;
	int m_svn_commit_wait_sec;
	CData m_temp_path;

	int m_cur_resource_num;
	int m_used_resource_count;
	std::map<CADDRINFO,int> m_client_resource_map;
};
