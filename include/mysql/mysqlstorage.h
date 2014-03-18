#ifndef C2CENT_STORAGE_MYSQLSTORAGE_H
#define C2CENT_STORAGE_MYSQLSTORAGE_H

#include <string>
#include "storage.h"
#include "mysqlresultset.h"
#include "db_operator_inf.h"

//class CDBOperator2;

class CMySqlStorage : public IStorage
{
private:
	static const uint32_t TRAN_STATE_INIT     = 0;
	static const uint32_t TRAN_STATE_START    = 1;
	static const uint32_t TRAN_STATE_COMMIT   = 2;
	static const uint32_t TRAN_STATE_ROLLBACK = 3;
	static const uint32_t TRAN_STATE_NONE = 3;

    IDBOperator* m_pOper;
    CMysqlResultSet m_ResultSet;
    std::string m_sErrMsg;
	uint32_t m_dwStat;
	int m_iErrno;
public:
	CMySqlStorage(IDBOperator* pOper = NULL);
    ~CMySqlStorage();

    void SetDBOperator(IDBOperator* pOper);
	IDBOperator* GetDBOperator();

	const std::string EscapeString(const std::string& sSrc);
public:
    virtual const char* GetErrMsg() const;
	virtual int GetErrno() const;
    //���´洢�е����� sStatement �Ǹ�����ʹ�õ��Զ���淶������key
    //�Ժ� mysql ����sql, ���� CacheCenter ���Ǹ�������
    virtual int Update(const std::string& sStatement);
    virtual int Update(const char* pszStatement);
	virtual int GetAffectedRows();
	//��ȡ���һ�β��������Ӧ���ݱ���������ֶε�ֵ
	virtual uint32_t GetLastInsertId();
    //����һ��sStatement ��Storage�л�ȡһ�����ݼ���һ����ά��
    virtual IResultSet& QueryForResultSet(const std::string& sStatement);
    virtual IResultSet& QueryForResultSet(const char* pszStatement);
	/**
	 * @fn  uint32_t BeginTransaction();
	 * ��������
	 *
	 * @return 0 �ɹ�
	 *         ��0 ʧ��
	 */
	virtual uint32_t BeginTransaction();
	/**
	 * @fn  uint32_t CommitTransaction();
	 * �ύ����
	 *
	 * @return 0 �ɹ�
	 *         ��0 ʧ��
	 */
	virtual uint32_t CommitTransaction();
	/**
	 * @fn  uint32_t RollBackTransaction();
	 * �ύ����
	 *
	 * @return 0 �ɹ�
	 *         ��0 ʧ��
	 */
	//�ع�����
	virtual uint32_t RollBackTransaction();
};


#endif
