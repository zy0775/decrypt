#pragma once

#include "ace/Task.h"

class CNetMsg;
class CFileHandler : public ACE_Task_Base
{
public:
	CFileHandler(CNetMsg* msg);
	virtual ~CFileHandler(void);
public:
	virtual int svc();
	virtual int close(u_long flags = 0);
private:
	CNetMsg* m_msg;
};
