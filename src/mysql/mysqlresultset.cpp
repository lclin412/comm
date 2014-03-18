
#include <stdlib.h>
#include "mysqlresultset.h"

extern "C" 
{
	#include "mysql.h"
}
#include <assert.h>
#include <iostream>


void CMysqlResultSet::Reset()
{
    m_bReady = true;
    m_iRowNo = -1;
}

void CMysqlResultSet::SetDBOperator(IDBOperator* pOper)
{
    m_pOper = pOper;
} 

void CMysqlResultSet::Close()
{
    if(m_pOper)
    	m_pOper->FreeQueryResult();
}

int CMysqlResultSet::GetColumnCount()
{
    assert(m_pOper);
    return m_pOper->GetFieldNum();
}

void CMysqlResultSet::SetReady(bool bVal)
{
    m_bReady = bVal;
}

int CMysqlResultSet::GetRecordCount()
{
    assert(m_pOper);
    return m_pOper->GetRowNum();
}

bool CMysqlResultSet::First()
{
    assert(m_pOper);

    if(m_pOper->GetRowNum() > 0)
    {
        m_iRowNo = 0;
        m_CurrRow = &(m_pOper->FetchRow());
    }

    return m_iRowNo > -1;
}

bool CMysqlResultSet::Next()
{
    assert(m_pOper);
    bool ret = false;

    if((m_iRowNo > -1) && (m_iRowNo < m_pOper->GetRowNum() - 1))
    {
        ret = true;
        m_iRowNo += 1;
        m_CurrRow = &(m_pOper->FetchRow());
    }
    
    return ret;
}

const char* CMysqlResultSet::GetString(uint32_t dwFieldIndex)
{
    assert(m_CurrRow);
    const char* pTmp = (*m_CurrRow)[dwFieldIndex];
	if(pTmp)
		return pTmp;

	return "";	
}

int CMysqlResultSet::GetInt(uint32_t dwFieldIndex)
{
	assert(m_CurrRow);

	if((*m_CurrRow)[dwFieldIndex])
		return atoi((*m_CurrRow)[dwFieldIndex]);

	return 0;
}

uint32_t CMysqlResultSet::GetUInt(uint32_t dwFieldIndex)
{
	assert(m_CurrRow);

	if((*m_CurrRow)[dwFieldIndex])
		return strtoul((*m_CurrRow)[dwFieldIndex], NULL, 10);

	return 0;
}

int64_t CMysqlResultSet::GetBigInt(uint32_t dwFieldIndex)
{
	assert(m_CurrRow);

	if((*m_CurrRow)[dwFieldIndex])
		return strtoll((*m_CurrRow)[dwFieldIndex], NULL, 10);

	return 0;
}

uint64_t CMysqlResultSet::GetBigUInt(uint32_t dwFieldIndex)
{
	assert(m_CurrRow);

	if((*m_CurrRow)[dwFieldIndex])
		return strtoull((*m_CurrRow)[dwFieldIndex], NULL, 10);

	return 0;
}

const char* CMysqlResultSet::GetString(const std::string& sFieldName)
{
	int iFieldIndex = m_pOper->GetFieldIndex(sFieldName.c_str());
	if(iFieldIndex < 0)
    	return "";
    else
    	return GetString(iFieldIndex);
}

int CMysqlResultSet::GetInt(const std::string& sFieldName)
{
	int iFieldIndex = m_pOper->GetFieldIndex(sFieldName.c_str());
	if(iFieldIndex < 0)
    	return 0;
    else
    	return GetInt(iFieldIndex);
}

uint32_t CMysqlResultSet::GetUInt(const std::string& sFieldName)
{
	int iFieldIndex = m_pOper->GetFieldIndex(sFieldName.c_str());
	if(iFieldIndex < 0)
    	return 0;
    else
    	return GetUInt(iFieldIndex);
}

int64_t CMysqlResultSet::GetBigInt(const std::string& sFieldName)
{
	int iFieldIndex = m_pOper->GetFieldIndex(sFieldName.c_str());
	if(iFieldIndex < 0)
    	return 0;
    else
    	return GetBigInt(iFieldIndex);
}

uint64_t CMysqlResultSet::GetBigUInt(const std::string& sFieldName)
{
	int iFieldIndex = m_pOper->GetFieldIndex(sFieldName.c_str());
	if(iFieldIndex < 0)
    	return 0;
    else
    	return GetBigUInt(iFieldIndex);
}

