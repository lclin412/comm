#ifndef C2CENT_STORAGE_IRESULTSET_H
#define C2CENT_STORAGE_IRESULTSET_H

#include <string>
#include <stdint.h>


class IResultSet
{
public:
    virtual ~IResultSet(){};
public:
    //关闭数据集
    virtual void Close() = 0;
    //获取数据集的列数
    virtual int GetColumnCount() = 0;
    //获取数据集的记录数
    virtual int GetRecordCount() = 0;
    //将游标移动到第一条记录的位置，如果没有记录那么返回false，相反返回 true
    virtual bool First() = 0;
    //将游标移动到下一条记录，如果已到数据集的尾部那么返回false，相反返回true
    virtual bool Next() = 0;
    //以字符串的形式获取指定索引字段的值
    virtual const char* GetString(uint32_t dwFieldIndex) = 0;
    //以int的形式获取指定索引字段的值
    virtual int GetInt(uint32_t dwFieldIndex) = 0;
    //以字符串的形式获取指定名称字段的值
    virtual const char* GetString(const std::string& sFieldName) = 0;
    //以int的形式获取指定名称字段的值
    virtual int GetInt(const std::string& sFieldName) = 0;
    //结果集是否准备妥当
    virtual bool IsReady() = 0;
    //以bigint的形式获取指定索引字段的值
    virtual int64_t GetBigInt(uint32_t dwFieldIndex) = 0;
    //以bigint的形式获取指定名称字段的值
    virtual int64_t GetBigInt(const std::string& sFieldName) = 0;
    //以biguint的形式获取指定索引字段的值
    virtual uint64_t GetBigUInt(uint32_t dwFieldIndex) = 0;
    //以biguint的形式获取指定名称字段的值
    virtual uint64_t GetBigUInt(const std::string& sFieldName) = 0;
    //以uint32_t的形式获取指定索引字段的值
    virtual uint32_t GetUInt(uint32_t dwFieldIndex) = 0;
    //以uint32_t的形式获取指定名称字段的值
    virtual uint32_t GetUInt(const std::string& sFieldName) = 0;
};


#endif

