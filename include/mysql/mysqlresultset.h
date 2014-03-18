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
    //������Ƿ�׼���׵�
    virtual bool IsReady(){return m_bReady;};
    //�ر����ݼ�
    virtual void Close();
    //��ȡ���ݼ�������
    virtual int GetColumnCount();
    //��ȡ���ݼ��ļ�¼��
    virtual int GetRecordCount();
    //���α��ƶ�����һ����¼��λ�ã����û�м�¼��ô����false���෴���� true
    virtual bool First();
    //���α��ƶ�����һ����¼������ѵ����ݼ���β����ô����false���෴����true
    virtual bool Next();
    //���ַ�������ʽ��ȡָ�������ֶε�ֵ
    virtual const char* GetString(uint32_t dwFieldIndex);
    //��int����ʽ��ȡָ�������ֶε�ֵ
    virtual int GetInt(uint32_t dwFieldIndex);
    //��int����ʽ��ȡָ�������ֶε�ֵ
    virtual int GetInt(const std::string& sFieldName);
    //��uint32_t����ʽ��ȡָ�������ֶε�ֵ
    virtual uint32_t GetUInt(uint32_t dwFieldIndex);
    //��uint32_t����ʽ��ȡָ�������ֶε�ֵ
    virtual uint32_t GetUInt(const std::string& sFieldName);
    //���ַ�������ʽ��ȡָ�������ֶε�ֵ
    virtual const char* GetString(const std::string& sFieldName);
    //��bigint����ʽ��ȡָ�������ֶε�ֵ
    virtual int64_t GetBigInt(uint32_t dwFieldIndex);
    //��bigint����ʽ��ȡָ�������ֶε�ֵ
    virtual int64_t GetBigInt(const std::string& sFieldName);
    //��biguint����ʽ��ȡָ�������ֶε�ֵ
    virtual uint64_t GetBigUInt(uint32_t dwFieldIndex);
    //��biguint����ʽ��ȡָ�������ֶε�ֵ
    virtual uint64_t GetBigUInt(const std::string& sFieldName);
        
private:
    IDBOperator* m_pOper;
    bool m_bReady;
    int  m_iRowNo;
    const MYSQL_ROW *m_CurrRow;        
};

#endif

