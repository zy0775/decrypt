#if !defined(AFX_FTPINTERFACE_H__E5BA3ADD_22EB_4199_BF29_C52984854524__INCLUDED_)
#define AFX_FTPINTERFACE_H__E5BA3ADD_22EB_4199_BF29_C52984854524__INCLUDED_

	#ifdef MFCFTP_EXPORTS
		#define DLL_DEFINE __declspec (dllexport)
	#else
		#define DLL_DEFINE __declspec (dllimport)
	#endif

class CCommonFtp
{
public:
	virtual int FtpConnection(const char * pstrServer, const char * pstrUserName = 0, 
		const char * pstrPassword = 0, int nPort = 0, int bPassive = 0) =0;
	virtual int SetCurrentDirectory(const char * pstrDirName )= 0;
	virtual const char * GetCurrentDirectory()=0;
	virtual int CreateDirectory( const char * pstrDirName )= 0;
	virtual int PutFile( const char * pstrLocalFile, const char * pstrRemoteFile, 
		int dwFlags = 2, int dwContext = 1 ) =0;
	virtual int GetFile( const char *  pstrRemoteFile, const char *  pstrLocalFile, int bFailIfExists = 1, 
		int dwAttributes = 0x00000080, int dwFlags = 2, int dwContext = 1 )=0;
	virtual void Close( )=0;
};

DLL_DEFINE CCommonFtp* GetCommonFtp();

#endif 
