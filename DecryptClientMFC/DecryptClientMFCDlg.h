// DecryptClientMFCDlg.h : 头文件
//

#pragma once

#include "ProtocolData.h"

// CDecryptClientMFCDlg 对话框
class CDecryptClientMFCDlg : public CDialog
{
// 构造
public:
	CDecryptClientMFCDlg(CWnd* pParent = NULL);	// 标准构造函数

// 对话框数据
	enum { IDD = IDD_DECRYPTCLIENTMFC_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持


// 实现
protected:
	HICON m_hIcon;

	// 生成的消息映射函数
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
private:
	int m_selected_count;
	CData m_server_ip;
	int m_server_port;
	int m_file_recover;
	CData m_cur_directory;
	int m_ftp_port;
	CData m_ftp_user;
	CData m_ftp_password;
	CData m_guid;
	int m_resource_seq;
	CData m_svn_url;
private:
	void AddFilenameToList(const char* path);
	int AddPathToList(const char* path);
	int UploadToFTP(CElement& elemcommit);
public:
	afx_msg BOOL OnCopyData(CWnd* pWnd, COPYDATASTRUCT* pCopyDataStruct);
	afx_msg void OnBnClickedCheckAll();
	afx_msg void OnLvnItemchangedListFiles(NMHDR *pNMHDR, LRESULT *pResult);
public:
	static int ExecSvnCmd(const char* cmd,int timeoutsec);
	afx_msg void OnBnClickedOk();
};
