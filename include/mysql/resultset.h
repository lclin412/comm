#ifndef C2CENT_STORAGE_IRESULTSET_H
#define C2CENT_STORAGE_IRESULTSET_H

#include <string>
#include <stdint.h>


class IResultSet
{
public:
    virtual ~IResultSet(){};
public:
    //�ر����ݼ�
    virtual void Close() = 0;
    //��ȡ���ݼ�������
    virtual int GetColumnCount() = 0;
    //��ȡ���ݼ��ļ�¼��
    virtual int GetRecordCount() = 0;
    //���α��ƶ�����һ����¼��λ�ã����û�м�¼��ô����false���෴���� true
    virtual bool First() = 0;
    //���α��ƶ�����һ����¼������ѵ����ݼ���β����ô����false���෴����true
    virtual bool Next() = 0;
    //���ַ�������ʽ��ȡָ�������ֶε�ֵ
    virtual const char* GetString(uint32_t dwFieldIndex) = 0;
    //��int����ʽ��ȡָ�������ֶε�ֵ
    virtual int GetInt(uint32_t dwFieldIndex) = 0;
    //���ַ�������ʽ��ȡָ�������ֶε�ֵ
    virtual const char* GetString(const std::string& sFieldName) = 0;
    //��int����ʽ��ȡָ�������ֶε�ֵ
    virtual int GetInt(const std::string& sFieldName) = 0;
    //������Ƿ�׼���׵�
    virtual bool IsReady() = 0;
    //��bigint����ʽ��ȡָ�������ֶε�ֵ
    virtual int64_t GetBigInt(uint32_t dwFieldIndex) = 0;
    //��bigint����ʽ��ȡָ�������ֶε�ֵ
    virtual int64_t GetBigInt(const std::string& sFieldName) = 0;
    //��biguint����ʽ��ȡָ�������ֶε�ֵ
    virtual uint64_t GetBigUInt(uint32_t dwFieldIndex) = 0;
    //��biguint����ʽ��ȡָ�������ֶε�ֵ
    virtual uint64_t GetBigUInt(const std::string& sFieldName) = 0;
    //��uint32_t����ʽ��ȡָ�������ֶε�ֵ
    virtual uint32_t GetUInt(uint32_t dwFieldIndex) = 0;
    //��uint32_t����ʽ��ȡָ�������ֶε�ֵ
    virtual uint32_t GetUInt(const std::string& sFieldName) = 0;
};


#endif

