#include "ace/ACE.h"
#include "ace/OS.h"
#include "ZxLog.h"
#include "common/Random.h"
#include "../../../zxvnms/550/Src/vqd/FTPModule/FTPModule.h"
#include "MsgService.h"
#include "FileHandler.h"
#include "shellapi.h"

CMsgService* CMsgService::m_instance = 0;

CMsgService::CMsgService(void)
{
	m_listen_port = 7777;
	m_ftp_port = 77;
	m_resource_count = 0;
	m_cur_resource_num = 0;
	m_used_resource_count = 0;
	m_svn_commit_wait_sec = 0;
}

CMsgService::~CMsgService(void)
{
}

CMsgService* CMsgService::Instance()
{
	if (m_instance == 0)
	{
		m_instance = new CMsgService;
	}
	return m_instance;
}

void CMsgService::CloseSingleton()
{
	if (m_instance != 0 )
	{
		delete m_instance;
		m_instance = 0;
	}
}

int CMsgService::OnNetMsg(const CADDRINFO &AddrInfo,CNetMsg *pMsg)
{
	switch(pMsg->GetCmdCode())
	{
	case 77771:
		{
			int file_count = pMsg->GetXML()->GetValueStep("Message/resource","filecount").convertInt();

			if (m_used_resource_count+file_count >= m_resource_count)
			{
				pMsg->Response(-1,"server error - resource reach maximum");
				return 0;
			}

			CElement elem_ftp = pMsg->GetResponseXML()->AddElement("Message/ftp");
			elem_ftp.SetAttribute("port",m_ftp_port);
			elem_ftp.SetAttribute("user",m_ftp_user.c_str());
			elem_ftp.SetAttribute("pwd",m_ftp_password.c_str());
 
			CElement elem_resource = pMsg->GetResponseXML()->AddElement("Message/resource");
			elem_resource.SetAttribute("svnurl",m_svn_url.c_str());
			CData guid = GenGUID();
			elem_resource.SetAttribute("guid",guid.c_str());
			elem_resource.SetAttribute("number",m_cur_resource_num);
			m_used_resource_count += file_count;
			m_client_resource_map[AddrInfo] = file_count;
			LOG_INFO("client " << AddrInfo.GetIP() << ":" << AddrInfo.GetPort()
				<< " request resource success,file count " << file_count << ",start resource number " << m_cur_resource_num
				<< ",used resource count " << m_used_resource_count << ",total resource count " << m_resource_count);
			m_cur_resource_num = (m_cur_resource_num+file_count)%m_resource_count;

			pMsg->Response();
		}
		break;
	case 77773:
		{
			CFileHandler* handler = new CFileHandler(pMsg);
			handler->activate();
		}
		break;
	default:
		break;
	}
	return 0;
}

int CMsgService::OnConnect(const CADDRINFO &AddrInfo)
{
	return 0;
}

int CMsgService::OnAccept(const CADDRINFO &AddrInfo)
{
	return 0;
}

int CMsgService::OnClose(const CADDRINFO &AddrInfo)
{
	std::map<CADDRINFO,int>::iterator pos = m_client_resource_map.find(AddrInfo);
	if (pos != m_client_resource_map.end())
	{
		int resource_count = pos->second;
		m_used_resource_count -= resource_count;
		m_client_resource_map.erase(pos);
		LOG_INFO("release client " << AddrInfo.GetIP() << ":" << AddrInfo.GetPort()
			<< " resource count " << resource_count
			<< ",used resource count " << m_used_resource_count << ",total resource count " << m_resource_count);
	}
	return 0;
}

int CMsgService::Start()
{
	CProtocolData cfg;
	int ret = cfg.Parse("decrypt_server_config.xml");
	if (ret != 0)
	{
		LOG_ERROR("load config from decrypt_server_config.xml fail");
		return -1;
	}
	m_listen_port = cfg.GetValueStep("decrypt_server_config/server/listen_port").convertInt();
	m_resource_count = cfg.GetValueStep("decrypt_server_config/server/resource_count").convertInt();
	m_ftp_port = cfg.GetValueStep("decrypt_server_config/ftp/port").convertInt();
	m_ftp_user = cfg.GetValueStep("decrypt_server_config/ftp/user");
	m_ftp_password = cfg.GetValueStep("decrypt_server_config/ftp/pwd");
	m_svn_url = cfg.GetValueStep("decrypt_server_config/svn/url");
	m_svn_commit_wait_sec = cfg.GetValueStep("decrypt_server_config/svn/commit_wait_sec").convertInt();
	if (m_svn_commit_wait_sec < 2)
	{
		m_svn_commit_wait_sec = 2;
	}

	char cur_dir[256];
	memset(cur_dir,0,256);
	::GetModuleFileName(0,cur_dir,256);
	CData temp_path(cur_dir); 
	temp_path = temp_path.substring(0,temp_path.find_last_of("\\",temp_path.length()-1));
	temp_path += "\\TempServer";
	m_temp_path = temp_path;

	CData cmd_checkout("checkout ");
	cmd_checkout += m_svn_url;
	cmd_checkout += " ";
	cmd_checkout += m_temp_path;
	cmd_checkout += " --quiet";
	ret = ExecSvnCmd(cmd_checkout,60);
	if (ret != 0)
	{
		return -1;
	}

	ret = FTP_AddUser(m_ftp_user.c_str(),m_ftp_password.c_str(),m_temp_path.c_str());
	if (ret != 0)
	{
		return -1;
	}
	ret = FTP_AddAllPurview(m_ftp_user.c_str());
	if (ret != 0 )
	{
		return -1;
	}
	LOG_INFO("start ftp at port:" << m_ftp_port << ",user:" << m_ftp_user.c_str()
		<< ",pwd:" << m_ftp_password.c_str() << ",root:" << m_temp_path.c_str());
	ret = FTP_Start(m_ftp_port);
	if (ret != 0 )
	{
		return -1;
	}

	PUBLISHER::GetIPublisher()->RegNetMsg(77771,this);
	PUBLISHER::GetIPublisher()->RegNetMsg(77773,this);
	PUBLISHER::GetIPublisher()->RegNetMsg(70001,this);

	CADDRINFO listen_addr("127.0.0.1",m_listen_port);
	return PUBLISHER::GetIPublisher()->BeginServer(listen_addr);
}

int CMsgService::UploadToSvn(CNetMsg* msg)
{
	CData filelist("");
	int copysuccess = 0;
	CProtocolData* pro = msg->GetXML();
	pro->Query("file");
	CElement elem = pro->Next();
	while(elem != 0)
	{
		int uploadsuccess = elem.GetAttribute("uploadsuccess").convertInt();
		if (uploadsuccess)
		{
			CData remote_name = elem.GetAttribute("remotename");
			CData cpp_name = elem.GetAttribute("cppname");
			CData src =  m_temp_path;
			src += "\\";
			src += remote_name;
			CData dst = m_temp_path;
			dst += "\\";
			dst += cpp_name;

			if (!::CopyFile(src.c_str(),dst.c_str(),FALSE))
			{
				LOG_ERROR("copy " << src.c_str() << " to " << dst.c_str() << " fail");
			}
			else
			{
				++copysuccess;
			}

			if (!filelist.empty())
			{
				filelist += "*";
			}
			filelist += dst;
		}
		elem = pro->Next();
	}
	LOG_INFO("copy " << copysuccess << " files success");
	int uploadcount = pro->GetValueStep("Message/commit","uploadcount").convertInt();

	SHELLEXECUTEINFO ShExecInfo = {0};
	ShExecInfo.cbSize = sizeof(SHELLEXECUTEINFO);
	ShExecInfo.fMask = SEE_MASK_NOCLOSEPROCESS;
	ShExecInfo.hwnd = NULL;
	ShExecInfo.lpVerb = NULL;
	ShExecInfo.lpFile = "tortoiseproc";
	CData param("/command:commit /path:\"");
	param += filelist;
	param += "\" /logmsg:\"";
	param += "0123456789";
	param += "\" /closeonend:1";
	LOG_INFO(param.c_str());
	ShExecInfo.lpParameters = param.c_str();
	ShExecInfo.lpDirectory = NULL;
	ShExecInfo.nShow = SW_HIDE;
	ShExecInfo.hInstApp = NULL;    
	if (!ShellExecuteEx(&ShExecInfo))
	{
		LOG_ERROR("ShellExecuteEx tortoiseproc fail");
		msg->Response(-1,"server error - server commit fail");
		return -2;
	}

	::Sleep(1000);
	DWORD tortoiseproc_pid = ::GetProcessId(ShExecInfo.hProcess);
	HWND hcommit = ReturnWnd(tortoiseproc_pid);
	if (NULL == hcommit)
	{
		LOG_ERROR("get tortoiseproc commit hwnd null");
		msg->Response(-2,"server error - server commit get window fail");
		return -2;
	}
	LOG_INFO("send ok command to commit dialog");
	::SendMessage(hcommit,WM_COMMAND,MAKELONG(1,BN_CLICKED),(long)hcommit);
	int commit_wait_sec = m_svn_commit_wait_sec+uploadcount;
	if (commit_wait_sec > 120)
	{
		commit_wait_sec = 120;
	}
	if (::WaitForSingleObject(ShExecInfo.hProcess,commit_wait_sec*1000) == WAIT_TIMEOUT)
	{
		LOG_ERROR("wait tortoiseproc exit time out " << commit_wait_sec);
		::TerminateProcess(ShExecInfo.hProcess,0);
		msg->Response(-3,"server error - server commit confirm fail");
		return -3;
	}
	LOG_INFO("tortoiseproc commit end");

	msg->Response(0);
	return 0;
}

CData CMsgService::GenGUID()
{
	time_t tmTime;
	tmTime = ACE_OS::time(NULL);
	tm tmGather = *ACE_OS::localtime(&tmTime);

	CRandom random;
	ACE_UINT32 r = random.RandomUInt32();

	char temp[32];
	memset(temp,0,32);
	ACE_OS::sprintf(temp,"%d%02d%02d%02d%02d%02d_%08x",tmGather.tm_year+1900,
		tmGather.tm_mon+1,tmGather.tm_mday,tmGather.tm_hour,tmGather.tm_min,
		tmGather.tm_sec,r);

	return temp;
}

int CMsgService::ExecSvnCmd(CData cmd,int timeoutsec)
{
	time_t t_start = time(0);
	SHELLEXECUTEINFO ShExecInfo = {0};
	ShExecInfo.cbSize = sizeof(SHELLEXECUTEINFO);
	ShExecInfo.fMask = SEE_MASK_NOCLOSEPROCESS;
	ShExecInfo.hwnd = NULL;
	ShExecInfo.lpVerb = NULL;
	ShExecInfo.lpFile = "svn";        
	ShExecInfo.lpParameters = cmd.c_str();
	ShExecInfo.lpDirectory = NULL;
	ShExecInfo.nShow = SW_HIDE;
	ShExecInfo.hInstApp = NULL;    
	if (!ShellExecuteEx(&ShExecInfo))
	{
		LOG_ERROR("ShellExecuteEx fail");
		return -1;
	}
	DWORD dwMilliseconds = timeoutsec * 1000;
	if (WaitForSingleObject(ShExecInfo.hProcess,dwMilliseconds) == WAIT_TIMEOUT)
	{
		LOG_ERROR("ShellExecuteEx time out " << dwMilliseconds);
		TerminateProcess(ShExecInfo.hProcess, 0);
		return -1;
	}
	DWORD dwExitCode;
	if (!GetExitCodeProcess(ShExecInfo.hProcess,&dwExitCode))
	{
		LOG_ERROR("GetExitCodeProcess fail");
		return -1;
	}
	if (dwExitCode != 0)
	{
		LOG_ERROR("GetExitCodeProcess exit code " << dwExitCode);
	}
	else
	{
		time_t t_end = time(0);
		time_t t_used = t_end - t_start;
		LOG_INFO("exec svn " << cmd.c_str() << " use sec " << t_used);
	}
	return dwExitCode;
}

BOOL CALLBACK CMsgService::EnumFunc(HWND hwnd,LPARAM lParam)
{
	EnumFunArg* pArg = (LPEnumFunArg)lParam;     
	DWORD processId;
	GetWindowThreadProcessId(hwnd,&processId);
	if(processId == pArg->dwProcessId)    
	{     
		pArg->hWnd = hwnd;
		char temp[2048] = {0};
		::GetWindowText(hwnd,temp,2048);
		if (CData(temp).find(" - Commit - TortoiseSVN") != CDATA_NPOS)
		{
			return FALSE;
		}     
	}  
	return TRUE;     
}

HWND CMsgService::ReturnWnd(DWORD processID)
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