#include "FileHandler.h"
#include "IPublisher.h"
#include "ZxLog.h"
#include "MsgService.h"

CFileHandler::CFileHandler(CNetMsg* msg)
{
	m_msg = new CNetMsg(*msg);
}

CFileHandler::~CFileHandler(void)
{
	delete m_msg;
}

int CFileHandler::svc()
{
	CMsgService::Instance()->UploadToSvn(m_msg);
	return 0;
}

int CFileHandler::close(u_long flags)
{
	delete this;
	return 0;
}