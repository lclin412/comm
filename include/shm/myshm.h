#ifndef _MY_SHM_H_
#define _MY_SHM_H_

#include <stdint.h>
#include <string>
#include <stdio.h>

class MyShm
{
public:
	MyShm()
	{
		m_Init = false;
		m_dwShmSize = 0;
		m_pHead = 0;
		m_dwShmKey = 0;
		m_dwShmId = 0;
		m_ErrMsg[0] = '\0';
	}

	~MyShm()
	{
		m_Init = false;
		m_dwShmSize = 0;
		m_dwShmId = 0;
		m_dwShmKey = 0;

		DetachShm();
	}

	int InitShm(int iShmKey, int iSize, void*& pMem, int iFlag = 0666);

	void ClearShm();
	bool DetachShm();

	int GetShmKey()
	{
		return m_dwShmKey;
	}
	char * GetAttachAddress()
	{
		return m_pHead;
	}
	int GetShmSize()
	{
		return m_dwShmSize;
	}
	std::string GetErrMsg()
	{
		return std::string(m_ErrMsg);
	}

private:
	uint32_t m_dwShmSize; //大小
	char* m_pHead;	//头指针
	uint32_t m_dwShmKey;
	bool m_Init;
	int m_dwShmId;
	char m_ErrMsg[256];
};

//定义共享内存的头和数据结构体

#define INDEX_SHMEM_LEFT	0
#define INDEX_SHMEM_RIGHT	1

#define ID_SHMEM_HEAD 60001
#define ID_SHMEM_LEFT 60002
#define ID_SHMEM_RIGHT 60003

//共享内存头定义
//设置一字节对齐

#pragma pack (1)

class CMemHead
{
public:
	unsigned int version;	//版本号
	unsigned int time;		//时间戳
	unsigned int index;		//对应的数据是哪一块LEFT 还是 RIGHT
	unsigned int imax;		//最大记录的条数
	unsigned int reserved2;
};

//单条记录
class CMemUnit
{
public:
	unsigned int version;	//版本号
	unsigned int time;		//时间戳
	unsigned int port;		//端口
	unsigned int hash;		//hash的值
	char szAppName[20];
	char szIpList[1024];
};

#pragma pack ()

#endif

