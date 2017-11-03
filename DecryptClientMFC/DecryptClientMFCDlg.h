// DecryptClientMFCDlg.h : ͷ�ļ�
//

#pragma once

#include "ProtocolData.h"

// CDecryptClientMFCDlg �Ի���
class CDecryptClientMFCDlg : public CDialog
{
// ����
public:
	CDecryptClientMFCDlg(CWnd* pParent = NULL);	// ��׼���캯��

// �Ի�������
	enum { IDD = IDD_DECRYPTCLIENTMFC_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV ֧��


// ʵ��
protected:
	HICON m_hIcon;

	// ���ɵ���Ϣӳ�亯��
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
