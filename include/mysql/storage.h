#ifndef C2CENT_STORAGE_ISTORAGE_H
#define C2CENT_STORAGE_ISTORAGE_H 

#include <string>
#include "resultset.h"

class IStorage
{
public:
	virtual ~IStorage(){};
public:
	//返回storage处理的错误信息
	virtual const char* GetErrMsg() const = 0;
	virtual int GetErrno() const = 0;
	//更新存储中的数据 sStatement 是更数据使用的自定义规范的语句或key。对和 mysql 就是sql, 对于 CacheCenter 就是更新数据
	virtual int Update(const std::string& sStatement) = 0;
	virtual int Update(const char* pszStatement) = 0;
	//更新操作影响的记录数
	virtual int GetAffectedRows() = 0;
	//获取最近一次插入操作对应数据表的自增长字段的值
	virtual uint32_t GetLastInsertId() = 0; 
	//根据一个sStatement 从Storage中获取一个数据集，一个二维表
	virtual IResultSet& QueryForResultSet(const std::string& sStatement) = 0;
	virtual IResultSet& QueryForResultSet(const char* pszStatement) = 0;
	/** 
	 * @fn  uint32_t BeginTransaction();
	 * 启动事务
	 * 
	 * @return 0 成功
	 *         非0 失败
	 */
	virtual uint32_t BeginTransaction() = 0;
	/** 
	 * @fn  uint32_t CommitTransaction();
	 * 提交事务
	 * 
	 * @return 0 成功
	 *         非0 失败
	 */
	virtual uint32_t CommitTransaction() = 0;
	/** 
	 * @fn  uint32_t RollBackTransaction();
	 * 提交事务
	 * 
	 * @return 0 成功
	 *         非0 失败
	 */
	//回滚事务
	virtual uint32_t RollBackTransaction() = 0;
};


#endif
