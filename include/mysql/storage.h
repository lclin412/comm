#ifndef C2CENT_STORAGE_ISTORAGE_H
#define C2CENT_STORAGE_ISTORAGE_H 

#include <string>
#include "resultset.h"

class IStorage
{
public:
	virtual ~IStorage(){};
public:
	//����storage����Ĵ�����Ϣ
	virtual const char* GetErrMsg() const = 0;
	virtual int GetErrno() const = 0;
	//���´洢�е����� sStatement �Ǹ�����ʹ�õ��Զ���淶������key���Ժ� mysql ����sql, ���� CacheCenter ���Ǹ�������
	virtual int Update(const std::string& sStatement) = 0;
	virtual int Update(const char* pszStatement) = 0;
	//���²���Ӱ��ļ�¼��
	virtual int GetAffectedRows() = 0;
	//��ȡ���һ�β��������Ӧ���ݱ���������ֶε�ֵ
	virtual uint32_t GetLastInsertId() = 0; 
	//����һ��sStatement ��Storage�л�ȡһ�����ݼ���һ����ά��
	virtual IResultSet& QueryForResultSet(const std::string& sStatement) = 0;
	virtual IResultSet& QueryForResultSet(const char* pszStatement) = 0;
	/** 
	 * @fn  uint32_t BeginTransaction();
	 * ��������
	 * 
	 * @return 0 �ɹ�
	 *         ��0 ʧ��
	 */
	virtual uint32_t BeginTransaction() = 0;
	/** 
	 * @fn  uint32_t CommitTransaction();
	 * �ύ����
	 * 
	 * @return 0 �ɹ�
	 *         ��0 ʧ��
	 */
	virtual uint32_t CommitTransaction() = 0;
	/** 
	 * @fn  uint32_t RollBackTransaction();
	 * �ύ����
	 * 
	 * @return 0 �ɹ�
	 *         ��0 ʧ��
	 */
	//�ع�����
	virtual uint32_t RollBackTransaction() = 0;
};


#endif
