#ifndef C2CENT_STORAGE_MYSQLRESULTSET_H
#define C2CENT_STORAGE_MYSQLRESULTSET_H

#include <string>
#include <stdint.h>
#include "resultset.h"
#include "db_operator_inf.h"

typedef char** MYSQL_ROW;


class CMysqlResultSet: public IResultSet
{
public:
    void Reset();
    void SetDBOperator(IDBOperator* pOper);
    void SetReady(bool bVal);
    
public:
    //结果集是否准备妥当
    virtual bool IsReady(){return m_bReady;};
    //关闭数据集
    virtual void Close();
    //获取数据集的列数
    virtual int GetColumnCount();
    //获取数据集的记录数
    virtual int GetRecordCount();
    //将游标移动到第一条记录的位置，如果没有记录那么返回false，相反返回 true
    virtual bool First();
    //将游标移动到下一条记录，如果已到数据集的尾部那么返回false，相反返回true
    virtual bool Next();
    //以字符串的形式获取指定索引字段的值
    virtual const char* GetString(uint32_t dwFieldIndex);
    //以int的形式获取指定索引字段的值
    virtual int GetInt(uint32_t dwFieldIndex);
    //以int的形式获取指定名称字段的值
    virtual int GetInt(const std::string& sFieldName);
    //以uint32_t的形式获取指定索引字段的值
    virtual uint32_t GetUInt(uint32_t dwFieldIndex);
    //以uint32_t的形式获取指定名称字段的值
    virtual uint32_t GetUInt(const std::string& sFieldName);
    //以字符串的形式获取指定名称字段的值
    virtual const char* GetString(const std::string& sFieldName);
    //以bigint的形式获取指定索引字段的值
    virtual int64_t GetBigInt(uint32_t dwFieldIndex);
    //以bigint的形式获取指定名称字段的值
    virtual int64_t GetBigInt(const std::string& sFieldName);
    //以biguint的形式获取指定索引字段的值
    virtual uint64_t GetBigUInt(uint32_t dwFieldIndex);
    //以biguint的形式获取指定名称字段的值
    virtual uint64_t GetBigUInt(const std::string& sFieldName);
        
private:
    IDBOperator* m_pOper;
    bool m_bReady;
    int  m_iRowNo;
    const MYSQL_ROW *m_CurrRow;        
};

#endif

