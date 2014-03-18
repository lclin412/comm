#include <sys/types.h>
#include <stdint.h>
#include <sys/shm.h>
#include <cstdio>
#include <cstring>
#include <cassert>
#include <cerrno>
#include "myshm.h"
#include <glog/logging.h>

char* GetShm(int iKey, int iSize, int iFlag, char * szErrMsg = NULL)
{
	int iShmID;
	char* sShm;

	if ((iShmID = shmget(iKey, iSize, iFlag)) < 0)
	{
		if (szErrMsg)
			snprintf(szErrMsg, 64, "shmget %d %d:%s", iKey, iSize,
					strerror(errno));
		return NULL;
	}

	if ((sShm = (char *) shmat(iShmID, NULL, 0)) == (char *) -1)
	{
		if (szErrMsg)
			snprintf(szErrMsg, 64, "shmat %d %d:%s", iKey, iSize,
					strerror(errno));
		return NULL;
	}
	return sShm;
}

bool DtShm(char * pShmAddr, char * szErrMsg)
{
	assert(pShmAddr);

	if (shmdt(pShmAddr) >= 0)
		return true;
	else
	{
		if (szErrMsg)
			snprintf(szErrMsg, 64, "shmdt %p:%s", pShmAddr, strerror(errno));
		return false;
	}
}

int MyShm::InitShm(int iShmKey, int iSize, void *& pMem, int iFlag)
{
	//已经初始化，且是同一块共享内存，直接返回ok
	if (m_Init == true && iShmKey == m_dwShmKey)
	{
		pMem = m_pHead;
		return 0;
	}

	//先以非创建的模式尝试
	m_dwShmId = shmget(iShmKey, iSize, iFlag & (~IPC_CREAT));
	if (m_dwShmId < 0)
	{
		m_dwShmId = shmget(iShmKey, iSize, iFlag | IPC_CREAT);
		if (m_dwShmId < 0)
		{
			snprintf(m_ErrMsg, 64, "shmget with create failed %d %d:%s",
					iShmKey, iSize, strerror(errno));
			LOG(ERROR)<<"shmget with create failed,ShmKey:"<<iShmKey<<",Size:"<<iSize<<",ErrMsg:"<<strerror(errno);
			return -1;
		}

	}

	m_pHead = (char*) shmat(m_dwShmId, NULL, 0);
	if (m_pHead <= 0)
	{
		snprintf(m_ErrMsg, 64, "shmat  failed %d %d:%s", iShmKey, iSize,
				strerror(errno));
		LOG(ERROR)<<"shmat failed,ShmKey:"<<iShmKey<<",Size:"<<iSize<<",ErrMsg:"<<strerror(errno);
		return -1;
	}

	pMem = m_pHead;

	m_dwShmKey = iShmKey;
	m_dwShmSize = iSize;

	m_Init = true;

	return 0;
}

bool MyShm::DetachShm()
{
	if (m_pHead != NULL)
	{
		shmdt(m_pHead);
		m_pHead = NULL;
	}

	return true;
}

void MyShm::ClearShm()
{
	if (m_dwShmId)
		shmctl(m_dwShmId, IPC_RMID, NULL);

}


