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
	uint32_t m_dwShmSize; //��С
	char* m_pHead;	//ͷָ��
	uint32_t m_dwShmKey;
	bool m_Init;
	int m_dwShmId;
	char m_ErrMsg[256];
};

//���干���ڴ��ͷ�����ݽṹ��

#define INDEX_SHMEM_LEFT	0
#define INDEX_SHMEM_RIGHT	1

#define ID_SHMEM_HEAD 60001
#define ID_SHMEM_LEFT 60002
#define ID_SHMEM_RIGHT 60003

//�����ڴ�ͷ����
//����һ�ֽڶ���

#pragma pack (1)

class CMemHead
{
public:
	unsigned int version;	//�汾��
	unsigned int time;		//ʱ���
	unsigned int index;		//��Ӧ����������һ��LEFT ���� RIGHT
	unsigned int imax;		//����¼������
	unsigned int reserved2;
};

//������¼
class CMemUnit
{
public:
	unsigned int version;	//�汾��
	unsigned int time;		//ʱ���
	unsigned int port;		//�˿�
	unsigned int hash;		//hash��ֵ
	char szAppName[20];
	char szIpList[1024];
};

#pragma pack ()

#endif

