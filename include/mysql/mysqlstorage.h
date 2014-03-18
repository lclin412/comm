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
    //更新存储中的数据 sStatement 是更数据使用的自定义规范的语句或key
    //对和 mysql 就是sql, 对于 CacheCenter 就是更新数据
    virtual int Update(const std::string& sStatement);
    virtual int Update(const char* pszStatement);
	virtual int GetAffectedRows();
	//获取最近一次插入操作对应数据表的自增长字段的值
	virtual uint32_t GetLastInsertId();
    //根据一个sStatement 从Storage中获取一个数据集，一个二维表
    virtual IResultSet& QueryForResultSet(const std::string& sStatement);
    virtual IResultSet& QueryForResultSet(const char* pszStatement);
	/**
	 * @fn  uint32_t BeginTransaction();
	 * 启动事务
	 *
	 * @return 0 成功
	 *         非0 失败
	 */
	virtual uint32_t BeginTransaction();
	/**
	 * @fn  uint32_t CommitTransaction();
	 * 提交事务
	 *
	 * @return 0 成功
	 *         非0 失败
	 */
	virtual uint32_t CommitTransaction();
	/**
	 * @fn  uint32_t RollBackTransaction();
	 * 提交事务
	 *
	 * @return 0 成功
	 *         非0 失败
	 */
	//回滚事务
	virtual uint32_t RollBackTransaction();
};


#endif
