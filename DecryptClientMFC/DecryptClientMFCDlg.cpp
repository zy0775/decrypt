// DecryptClientMFCDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "DecryptClientMFC.h"
#include "DecryptClientMFCDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#include <list>
#include "ace/Dirent.h"
#include "ace/OS_NS_dirent.h"
#include "ace/Dirent_Selector.h"
#include "ace/OS_NS_sys_stat.h"
#include "ace/Date_Time.h"
#include "ace/OS.h"
#include "ace/ACE.h"
#include "ace/Process_Mutex.h"
#include "IPublisher.h"
#include "FtpInterface.h"
#include "ZxLog.h"
#include "shellapi.h"

// CDecryptClientMFCDlg 对话框

CDecryptClientMFCDlg::CDecryptClientMFCDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CDecryptClientMFCDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	m_selected_count = 0;
	m_server_port = 0;
	m_file_recover = 0;
	m_ftp_port = 0;
	m_resource_seq = 0;
}

void CDecryptClientMFCDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CDecryptClientMFCDlg, CDialog)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	//}}AFX_MSG_MAP
	ON_WM_COPYDATA()
	ON_BN_CLICKED(IDC_CHECK_ALL, &CDecryptClientMFCDlg::OnBnClickedCheckAll)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_LIST_FILES, &CDecryptClientMFCDlg::OnLvnItemchangedListFiles)
	ON_BN_CLICKED(IDOK, &CDecryptClientMFCDlg::OnBnClickedOk)
END_MESSAGE_MAP()


// CDecryptClientMFCDlg 消息处理程序

BOOL CDecryptClientMFCDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// 设置此对话框的图标。当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	// TODO: 在此添加额外的初始化代码
	char cur_dir[MAX_PATH];
	memset(cur_dir,0,MAX_PATH);
	::GetModuleFileName(0,cur_dir,MAX_PATH);
	m_cur_directory = CData(cur_dir); 
	m_cur_directory = m_cur_directory.substring(0,m_cur_directory.find_last_of("\\",m_cur_directory.length()-1));

	CData cfgfile = m_cur_directory;
	cfgfile += "\\decrypt_client_config.xml";
	CProtocolData cfg;
	int ret = cfg.Parse(cfgfile.c_str());
	if (ret != 0)
	{
		CString s;
		s.Format("read configure file %s fail",cfgfile.c_str());
		AfxMessageBox(s);
		OnCancel();
		return FALSE;
	}
	m_server_ip = cfg.GetValueStep("decrypt_client_config/client/server_ip");
	m_server_port = cfg.GetValueStep("decrypt_client_config/client/server_port").convertInt();
	m_file_recover = cfg.GetValueStep("decrypt_client_config/client/file_recover").convertInt();

	CListCtrl* ctrl = (CListCtrl*)GetDlgItem(IDC_LIST_FILES);
	ctrl->SetExtendedStyle(LVS_EX_GRIDLINES|LVS_EX_FULLROWSELECT|LVS_EX_CHECKBOXES);

	ctrl->InsertColumn(0,"check",LVCFMT_CENTER,50);
	ctrl->InsertColumn(1,"file name",LVCFMT_LEFT,500);

	CButton* check = (CButton*)GetDlgItem(IDC_CHECK_ALL);
	check->SetCheck(1);
	AddPathToList(__argv[1]);

	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CDecryptClientMFCDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 用于绘制的设备上下文

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 使图标在工作矩形中居中
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 绘制图标
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialog::OnPaint();
	}
}

//当用户拖动最小化窗口时系统调用此函数取得光标显示。
//
HCURSOR CDecryptClientMFCDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}


BOOL CDecryptClientMFCDlg::OnCopyData(CWnd* pWnd, COPYDATASTRUCT* pCopyDataStruct)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	CData filepath((char*)pCopyDataStruct->lpData,pCopyDataStruct->cbData);
	AddPathToList(filepath.c_str());
	return CDialog::OnCopyData(pWnd, pCopyDataStruct);
}

void CDecryptClientMFCDlg::AddFilenameToList(const char* path)
{
	CListCtrl* ctrl = (CListCtrl*)GetDlgItem(IDC_LIST_FILES);
	int count = ctrl->GetItemCount();
	int index = ctrl->InsertItem(count,NULL);
	ctrl->SetItemText(index,1,path);
	ctrl->SetCheck(index);
}

int CDecryptClientMFCDlg::AddPathToList(const char* path)
{
	ACE_stat stattype;
	if(ACE_OS::lstat (path,&stattype) != 0)
	{
		return -1;
	}
	
	if((stattype.st_mode&S_IFMT) != S_IFDIR)
	{
		AddFilenameToList(path);
		return 0;
	}

	ACE_Dirent dir(path);
	for(dirent* directory;(directory = dir.read ()) != 0;)
	{
		if (ACE_OS::strcmp (directory->d_name,".") == 0
			|| ACE_OS::strcmp (directory->d_name,"..") == 0
			|| ACE_OS::strcmp (directory->d_name,".svn") == 0)
			continue;

		ACE_stat stat_buf;
		char tempPath[MAX_PATH];
		ACE_OS::sprintf(tempPath,"%s\\%s",path,directory->d_name);
		if (ACE_OS::lstat (tempPath,&stat_buf) == 0)
		{
			if((stat_buf.st_mode&S_IFMT) == S_IFDIR)
			{
				AddPathToList(tempPath);
			}
			else
			{
				AddFilenameToList(tempPath);
			}
		}
	}
	dir.close();
	return 0;
}

void CDecryptClientMFCDlg::OnBnClickedCheckAll()
{
	// TODO: 在此添加控件通知处理程序代码
	CButton* btn = (CButton*)GetDlgItem(IDC_CHECK_ALL);
	int check = btn->GetCheck();
	CListCtrl* ctrl = (CListCtrl*)GetDlgItem(IDC_LIST_FILES);
	for (int i = 0;i < ctrl->GetItemCount();i++)
	{
		ctrl->SetCheck(i,check);
	}
}

void CDecryptClientMFCDlg::OnLvnItemchangedListFiles(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	// TODO: 在此添加控件通知处理程序代码
	CListCtrl* ctrl = (CListCtrl*)GetDlgItem(IDC_LIST_FILES);
	int totalcount = ctrl->GetItemCount();
	m_selected_count = 0;
	for (int i = 0;i < totalcount;i++)
	{
		if (ctrl->GetCheck(i))
		{
			m_selected_count++;
		}
	}
	CString s;
	s.Format("total %d,selected %d,upload 0",totalcount,m_selected_count);
	GetDlgItem(IDC_STATIC_COUNT)->SetWindowText(s);
	*pResult = 0;
}

int CDecryptClientMFCDlg::UploadToFTP(CElement& elemcommit)
{
	CCommonFtp* ftp = GetCommonFtp();
	try
	{
		if (!ftp->FtpConnection(m_server_ip.c_str(),m_ftp_user.c_str(),m_ftp_password.c_str(),m_ftp_port,1))
		{
			CString s;
			s.Format("connect ftp %s:%d:%s:%s fail,",m_server_ip.c_str(),m_server_port,m_ftp_user,m_ftp_password);
			AfxMessageBox(s);
			return -1;
		}
	}
	catch (...)
	{
		CString s;
		s.Format("connect ftp %s:%d:%s:%s exception,",m_server_ip.c_str(),m_server_port,m_ftp_user,m_ftp_password);
		AfxMessageBox(s);
		return -1;
	}

	if (!ftp->SetCurrentDirectory("/"))
	{
		AfxMessageBox("set ftp root fail");
		ftp->Close();
		return -1;
	}

	if (!ftp->CreateDirectory(m_guid.c_str()))
	{
		CString s;
		s.Format("create ftp directory:%s fail",m_guid);
		AfxMessageBox(s);
		ftp->Close();
		return -1;
	}
	if (!ftp->SetCurrentDirectory(m_guid.c_str()))
	{
		CString s;
		s.Format("set ftp directory:%s fail",m_guid);
		AfxMessageBox(s);
		ftp->Close();
		return -1;
	}

	CListCtrl* ctrl = (CListCtrl*)GetDlgItem(IDC_LIST_FILES);
	int totalcount = ctrl->GetItemCount();
	int upload_success = 0;
	for (int i = 0;i < totalcount;i++)
	{
		if (ctrl->GetCheck(i))
		{
			CElement elemfile = elemcommit.AddSubElement("file");

			int resource = m_resource_seq+i;
			CString filepath = ctrl->GetItemText(i,1);
			int len = filepath.GetLength();
			int pos = filepath.ReverseFind('\\');
			CString filedir = filepath.Left(pos+1);
			CString filename = filepath.Right(len-pos-1);
			int pos1 = filename.ReverseFind('.');
			int len1 = filename.GetLength();
			CString rawname = filename.Left(pos1);
			CString extname = filename.Right(len1-pos1);
			CString remotename;
			remotename.Format("%s_%d%s",rawname,resource,extname);
			CString cppname;
			cppname.Format("%d.cpp",resource);
			int ret = ftp->PutFile(filepath.GetBuffer(),remotename.GetBuffer());
			LOG_INFO("upload file:" << filepath.GetBuffer() << " to:" << filename.GetBuffer() << " return:" << ret);

			elemfile.SetAttribute("uploadsuccess",ret);
			if (m_file_recover)
			{
				elemfile.SetAttribute("localname",filepath.GetBuffer());
			} 
			else
			{
				CString newfilepath = filedir+rawname+CString("_")+CString(m_guid.c_str())+extname;
				elemfile.SetAttribute("localname",newfilepath.GetBuffer());
			}
			CString remotepath = CString(m_guid.c_str())+CString("\\")+remotename;
			elemfile.SetAttribute("remotename",remotepath.GetBuffer());
			elemfile.SetAttribute("cppname",cppname.GetBuffer());
			++upload_success;
			CString s;
			s.Format("total %d,selected %d,upload %d",totalcount,m_selected_count,upload_success);
			GetDlgItem(IDC_STATIC_COUNT)->SetWindowText(s);
		}
	}
	elemcommit.SetAttribute("guid",m_guid.c_str());
	elemcommit.SetAttribute("selectcount",m_selected_count);
	elemcommit.SetAttribute("uploadcount",upload_success);

	ftp->Close();
	if (upload_success == 0)
	{
		AfxMessageBox("no file upload");
		return -1;
	}
	return upload_success;
}

int CDecryptClientMFCDlg::ExecSvnCmd(const char* cmd,int timeoutsec)
{
	SHELLEXECUTEINFO ShExecInfo = {0};
	ShExecInfo.cbSize = sizeof(SHELLEXECUTEINFO);
	ShExecInfo.fMask = SEE_MASK_NOCLOSEPROCESS;
	ShExecInfo.hwnd = NULL;
	ShExecInfo.lpVerb = NULL;
	ShExecInfo.lpFile = "svn";
	ShExecInfo.lpParameters = cmd;
	ShExecInfo.lpDirectory = NULL;
	ShExecInfo.nShow = SW_HIDE;
	ShExecInfo.hInstApp = NULL;    
	if (!ShellExecuteEx(&ShExecInfo))
	{
		LOG_ERROR("ShellExecuteEx fail cmd:" << cmd);
		return -1;
	}
	DWORD dwMilliseconds = timeoutsec * 1000;
	if (WaitForSingleObject(ShExecInfo.hProcess,dwMilliseconds) == WAIT_TIMEOUT)
	{
		TerminateProcess(ShExecInfo.hProcess,0);
		LOG_ERROR("ShellExecuteEx time out:" << timeoutsec);
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
		LOG_ERROR("GetExitCodeProcess exit code error:" << dwExitCode);
		return dwExitCode;
	}
	return 0;
}

void CDecryptClientMFCDlg::OnBnClickedOk()
{
	// TODO: 在此添加控件通知处理程序代码
	GetDlgItem(IDOK)->EnableWindow(FALSE);
	// 连接服务器
	CADDRINFO server_addr(m_server_ip.c_str(),m_server_port);
	int ret = PUBLISHER::GetIPublisher()->CreateLongConnect(server_addr);
	if (ret != 0)
	{
		CString s;
		s.Format("connect server %s:%d faid",m_server_ip.c_str(),m_server_port);
		AfxMessageBox(s);
		OnOK();
		return;
	}
	// 请求资源
	CNetMsg msg_req_resource;
	msg_req_resource.SetCmdCode(77771);
	CElement elem_resource = msg_req_resource.GetXML()->AddElement("Message/resource");
	elem_resource.SetAttribute("filecount",m_selected_count);
	CNetMsg msg_resource_resp;
	ret = PUBLISHER::GetIPublisher()->SendAndWaitNetMsg(server_addr,&msg_req_resource,&msg_resource_resp);
	if (ret != 0)
	{
		AfxMessageBox("send resource request message fail");
		OnOK();
		return;
	}
	int err = msg_resource_resp.GetErrorCode();
	if (err != 0)
	{
		AfxMessageBox(msg_resource_resp.GetErrorInfo().c_str());
		OnOK();
		return;
	}
	m_ftp_port = msg_resource_resp.GetXML()->GetValueStep("Message/ftp","port").convertInt();
	m_ftp_user = msg_resource_resp.GetXML()->GetValueStep("Message/ftp","user");
	m_ftp_password = msg_resource_resp.GetXML()->GetValueStep("Message/ftp","pwd");
	m_guid = msg_resource_resp.GetXML()->GetValueStep("Message/resource","guid");
	m_resource_seq = msg_resource_resp.GetXML()->GetValueStep("Message/resource","number").convertInt();
	m_svn_url = msg_resource_resp.GetXML()->GetValueStep("Message/resource","svnurl");
	// checkout所有资源
	CData temp_path = m_cur_directory;
	temp_path += "\\TempClient";
	CData cmd_checkout("checkout ");
	cmd_checkout += m_svn_url;
	cmd_checkout += " ";
	cmd_checkout += temp_path;
	cmd_checkout += " --quiet";
	ret = ExecSvnCmd(cmd_checkout.c_str(),60);
	if (ret != 0)
	{
		CString s;
		s.Format("exec svn cmd fail:%s",cmd_checkout.c_str());
		AfxMessageBox(s);
		OnOK();
		return;
	}
	// ftp上传文件,发送提交文件请求
	CNetMsg msg_req_commit;
	msg_req_commit.SetCmdCode(77773);
	CElement elem_commit = msg_req_commit.GetXML()->AddElement("Message/commit");
	int upload_count = UploadToFTP(elem_commit);
	if (upload_count < 0)
	{
		OnOK();
		return;
	}
	CNetMsg msg_commit_resp;
	ret = PUBLISHER::GetIPublisher()->SendAndWaitNetMsg(server_addr,&msg_req_commit,&msg_commit_resp,120+upload_count*2);
	if (ret != 0)
	{
		AfxMessageBox("send commit request message fail");
		OnOK();
		return;
	}
	err = msg_commit_resp.GetErrorCode();
	if (err != 0)
	{
		AfxMessageBox(msg_commit_resp.GetErrorInfo().c_str());
		OnOK();
		return;
	}

	CData cmd_update("update ");
	cmd_update += temp_path;
	cmd_update += " --quiet";
	ret = ExecSvnCmd(cmd_update.c_str(),60);
	if (ret != 0)
	{
		AfxMessageBox("update fail");
		OnOK();
		return;
	}

	int decryptsuccess = 0;
	CProtocolData* pro = msg_req_commit.GetXML();
	pro->Query("file");
	CElement elem = pro->Next();
	while(elem != 0)
	{
		int uploadsuccess = elem.GetAttribute("uploadsuccess").convertInt();
		if (uploadsuccess)
		{
			CData dst = elem.GetAttribute("localname");
			CData cpp_name = elem.GetAttribute("cppname");
			CData src =  temp_path;
			src += "\\";
			src += cpp_name;

			int ret = ::CopyFile(src.c_str(),dst.c_str(),FALSE);
			LOG_INFO("copy " << src.c_str() << " to " << dst.c_str() << " ret " << ret);
			if (ret)
			{
				++decryptsuccess;
			}
		}
		elem = pro->Next();
	}
	CString s;
	s.Format("decrypt %d files success",decryptsuccess);
	AfxMessageBox(s);
	OnOK();
}
