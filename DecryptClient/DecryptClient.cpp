// DecryptClient.cpp : 定义控制台应用程序的入口点。
//

#include "ace/ACE.h"
#include "ace/Process_Mutex.h"
#include "IPublisher.h"
#include "FtpInterface.h"
#include "shellapi.h"

#pragma comment(linker,"/subsystem:windows /entry:mainCRTStartup") 

int ExecSvnCmd(CData cmd,int timeoutsec)
{
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
		::MessageBox(NULL,"ShellExecuteEx fail","",MB_OK);
		return -1;
	}
	DWORD dwMilliseconds = timeoutsec * 1000;
	if (WaitForSingleObject(ShExecInfo.hProcess,dwMilliseconds) == WAIT_TIMEOUT)
	{
		TerminateProcess(ShExecInfo.hProcess, 0);
		::MessageBox(NULL,"ShellExecuteEx time out","",MB_OK);
		return -1;
	}
	DWORD dwExitCode;
	if (!GetExitCodeProcess(ShExecInfo.hProcess,&dwExitCode))
	{
		::MessageBox(NULL,"GetExitCodeProcess fail","",MB_OK);
		return -1;
	}
	if (dwExitCode != 0)
	{
		::MessageBox(NULL,"GetExitCodeProcess exit code error","",MB_OK);
		return dwExitCode;
	}
	return 0;
}

int ACE_TMAIN (int argc, ACE_TCHAR *agrv[])
{
	ACE_Process_Mutex* lock = new ACE_Process_Mutex("DecryptClient");
	if (lock)
	{
		int result = lock->tryacquire();
		if (result == -1)
		{
			return -1;
		}
	}

	if (argc != 2)
	{
		::MessageBox(NULL,"命令行参数错误","",MB_OK);
		return -1;
	}

	CCommonInit::Init();

	char cur_dir[256];
	memset(cur_dir,0,256);
	::GetModuleFileName(0,cur_dir,256);
	CData temp_cur(cur_dir); 
	temp_cur = temp_cur.substring(0,temp_cur.find_last_of("\\",temp_cur.length()-1));

	CData cfgfile = temp_cur;
	cfgfile += "\\decrypt_client_config.xml";
	CProtocolData cfg;
	int ret = cfg.Parse(cfgfile.c_str());
	if (ret != 0)
	{
		::MessageBox(NULL,"读取配置文件失败","",MB_OK);
		return -1;
	}
	CData server_ip = cfg.GetValueStep("decrypt_client_config/client/server_ip");
	int server_port = cfg.GetValueStep("decrypt_client_config/client/server_port").convertInt();

	CADDRINFO server_addr(server_ip.c_str(),server_port);
	ret = PUBLISHER::GetIPublisher()->CreateLongConnect(server_addr);
	if (ret != 0)
	{
		::MessageBox(NULL,"连接服务器失败","",MB_OK);
		return -1;
	}

	CData file_path(agrv[1]);
	CNetMsg msg_req_resource;
	msg_req_resource.SetCmdCode(77771);
	CElement elem_resource = msg_req_resource.GetXML()->AddElement("Message/resource");
	elem_resource.SetAttribute("filename",file_path.c_str());
	CNetMsg msg_resource_resp;
	ret = PUBLISHER::GetIPublisher()->SendAndWaitNetMsg(server_addr,&msg_req_resource,&msg_resource_resp);
	if (ret != 0)
	{
		::MessageBox(NULL,"发送资源请求协议失败","",MB_OK);
		return -1;
	}
	int err = msg_resource_resp.GetErrorCode();
	if (err != 0)
	{
		::MessageBox(NULL,msg_resource_resp.GetErrorInfo().c_str(),"",MB_OK);
		return -1;
	}
	int ftp_port = msg_resource_resp.GetXML()->GetValueStep("Message/ftp","port").convertInt();
	CData ftp_user = msg_resource_resp.GetXML()->GetValueStep("Message/ftp","user");
	CData ftp_password = msg_resource_resp.GetXML()->GetValueStep("Message/ftp","pwd");
	CData resource_guid = msg_resource_resp.GetXML()->GetValueStep("Message/resource","guid");
	CData resource_number = msg_resource_resp.GetXML()->GetValueStep("Message/resource","number");
	CData resource_svnurl = msg_resource_resp.GetXML()->GetValueStep("Message/resource","svnurl");

	CData temp_path = temp_cur;
	temp_path += "\\TempClient";

	CData cmd_checkout("checkout ");
	cmd_checkout += resource_svnurl;
	cmd_checkout += " ";
	cmd_checkout += temp_path;
	cmd_checkout += " --quiet";
	ret = ExecSvnCmd(cmd_checkout,60);
	if (ret != 0)
	{
		return -1;
	}

	CCommonFtp* ftp = GetCommonFtp();
	try
	{
		if (!ftp->FtpConnection(server_ip.c_str(),ftp_user.c_str(),ftp_password.c_str(),ftp_port,1))
		{
			::MessageBox(NULL,"连接FTP失败","",MB_OK);
			return -1;
		}
	}
	catch (...)
	{
		::MessageBox(NULL,"连接FTP失败","",MB_OK);
		return -1;
	}

	if (!ftp->SetCurrentDirectory("/"))
	{
		if (!ftp->CreateDirectory("/"))
		{
			::MessageBox(NULL,"获取FTP根目录失败","",MB_OK);
			return -1;
		}
		ftp->SetCurrentDirectory("/");
	}

	int pos1 = file_path.find_last_of("\\",file_path.length()-1);
	int pos2 = file_path.find_last_of(".",file_path.length()-1);
	CData file_dir = file_path.substring(0,pos1+1);
	CData file_name = file_path.substring(pos1+1,pos2);
	CData file_ext = file_path.substring(pos2);
	CData remote_file = file_name;
	remote_file += "_";
	remote_file += resource_guid;
	remote_file += file_ext;
	ret = ftp->PutFile(file_path.c_str(),remote_file.c_str());
	if (ret != 1)
	{
		::MessageBox(NULL,"文件上传FTP失败","",MB_OK);
		ftp->Close();
		return -1;
	}
	ftp->Close();

	CNetMsg msg_req_decrypt;
	msg_req_decrypt.SetCmdCode(77773);
	CElement elem_decrypt = msg_req_decrypt.GetXML()->AddElement("Message/decrypt");
	elem_decrypt.SetAttribute("newfilename",remote_file.c_str());
	CData cpp_name = resource_number;
	cpp_name += ".cpp";
	elem_decrypt.SetAttribute("resourcecppname",cpp_name.c_str());
	CNetMsg msg_decrypt_resp;
	ret = PUBLISHER::GetIPublisher()->SendAndWaitNetMsg(server_addr,&msg_req_decrypt,&msg_decrypt_resp,120);
	if (ret != 0)
	{
		::MessageBox(NULL,"发送解密请求协议失败","",MB_OK);
		return -1;
	}
	err = msg_decrypt_resp.GetErrorCode();
	if (err != 0)
	{
		::MessageBox(NULL,msg_decrypt_resp.GetErrorInfo().c_str(),"",MB_OK);
		return -1;
	}

	CData cmd_update("update ");
	cmd_update += temp_path;
	cmd_update += "\\";
	cmd_update += cpp_name;
	cmd_update += " --quiet";
	ret = ExecSvnCmd(cmd_update,60);
	if (ret != 0)
	{
		return -1;
	}

	int file_recover = cfg.GetValueStep("decrypt_client_config/client/file_recover").convertInt();
	CData obj_file_name = file_path;
	if (file_recover == 0)
	{
		obj_file_name = file_dir;
		obj_file_name += remote_file;
	}
	CData src_file_name = temp_path;
	src_file_name += "\\";
	src_file_name += cpp_name;
	if (!::CopyFile(src_file_name.c_str(),obj_file_name.c_str(),FALSE))
	{
		::MessageBox(NULL,"文件拷贝失败","",MB_OK);
		return -1;
	}
	::MessageBox(NULL,"解密成功","",MB_OK);
	
	return 0;
}

