#include "mysqlstorage.h"
#include <assert.h>
#include <iostream>
#include <string.h>
#include "db_operator_inf.h"
#include "inc_comm.h"

CMySqlStorage::CMySqlStorage(IDBOperator* pOper)
: m_pOper(pOper),m_dwStat(0)
{
}

CMySqlStorage::~CMySqlStorage()
{
}

void CMySqlStorage::SetDBOperator(IDBOperator* pOper)
{
	m_pOper = pOper;
}

IDBOperator* CMySqlStorage::GetDBOperator()
{
	return m_pOper;
}

const std::string CMySqlStorage::EscapeString(const std::string& sSrc)
{
	assert(m_pOper);
	return m_pOper->EscapeString(sSrc);
}

int CMySqlStorage::Update(const std::string& sStatement)
{
    return Update(sStatement.c_str());
}

const char* CMySqlStorage::GetErrMsg() const
{
    return m_sErrMsg.c_str();
}

int CMySqlStorage::GetErrno() const
{
	return m_iErrno;
}

int CMySqlStorage::Update(const char* pszStatement)
{
	int iRet = 0;

	m_sErrMsg = ""; //清除上一次操作的错误信息

	if(!m_pOper)
	{
	        m_sErrMsg = "Update failed. m_pOper is NULL. please check db config";
		m_iErrno = ERR_DB_INNER;
	        std::cout << m_sErrMsg << std::endl;
		return ERR_DB_INNER;
	}
	
	try
	{
		m_pOper->ExecSQL(pszStatement);
	}
	catch(IDBOperator::CDbException& e)
	{
		m_sErrMsg = e.what();
		m_iErrno = e.mysql_errno();
		iRet = ERR_DB_DBOPR;
		std::cout << m_sErrMsg << std::endl;
	}
	
    return iRet;
}

int CMySqlStorage::GetAffectedRows()
{
	int iRet = -1;
	
	if(!m_pOper)
	{
	        m_sErrMsg = "GetAffectedRows failed. m_pOper is NULL. please check db config";
		m_iErrno = ERR_DB_INNER;
	        std::cout << m_sErrMsg << std::endl;
		return -1;
	}
	
	try
	{
		iRet = m_pOper->GetAffectedRows();
	}
	catch(IDBOperator::CDbException& e)
	{
		m_sErrMsg = e.what();
		m_iErrno = e.mysql_errno();
		//iRet = ERR_DB_DBOPR;
		iRet = -1;
		std::cout << m_sErrMsg << std::endl;
	}
	return iRet;
}

IResultSet& CMySqlStorage::QueryForResultSet(const std::string& sStatement)
{
	return QueryForResultSet(sStatement.c_str()); 
}

IResultSet& CMySqlStorage::QueryForResultSet(const char* pszStatement)
{
	m_ResultSet.Reset();
	m_ResultSet.SetDBOperator(m_pOper);

	m_sErrMsg = ""; //清除上一次的错误消息
	
	
	if(!m_pOper)
	{
	        m_sErrMsg = "QueryForResultSet failed. m_pOper is NULL. please check db config";
		m_iErrno = ERR_DB_INNER;
	        m_ResultSet.SetReady(false);
	        std::cout << m_sErrMsg << std::endl;
		return m_ResultSet;
	}	
	try
	{
		m_pOper->Query(pszStatement);
	}
	catch(IDBOperator::CDbException& e)
	{
		m_sErrMsg = e.what();
		m_iErrno = e.mysql_errno();
		m_ResultSet.SetReady(false);
		std::cout << m_sErrMsg << std::endl;
	}
	return m_ResultSet;
}

uint32_t CMySqlStorage::GetLastInsertId()
{
	if(!m_pOper)
	{
	        m_sErrMsg = "GetLastInsertId failed. m_pOper is NULL. please check db config";
		m_iErrno = ERR_DB_INNER;
	        std::cout << m_sErrMsg << std::endl;
		return 0;
	}
	
	return m_pOper->GetInertId();
}

uint32_t CMySqlStorage::BeginTransaction()
{
	uint32_t dwRet = 0;
	if(m_dwStat != TRAN_STATE_START)
	{
		dwRet = Update("START TRANSACTION;");
		if(0 == dwRet)
		{
			m_dwStat = TRAN_STATE_START;	
			//设置db的状态为事务标记
			if(m_pOper)
				m_pOper->SetState(DB_TRAN_STATE_FLAG);
		}
	}
	return dwRet;
}

uint32_t CMySqlStorage::CommitTransaction()
{
	uint32_t dwRet = 0;
	if(m_dwStat == TRAN_STATE_START)
	{
		dwRet = Update("COMMIT;");
		if(0 == dwRet)
		{
			m_dwStat = TRAN_STATE_COMMIT;
			//设置db的状态为初使状态
			if(m_pOper)
				m_pOper->SetState(DB_STATE_NONE);
		}
	}
	return dwRet;
}

uint32_t CMySqlStorage::RollBackTransaction()
{
	uint32_t dwRet = 0;
	if(m_dwStat == TRAN_STATE_START)
	{
		dwRet = Update("ROLLBACK;");
		if(0 == dwRet)
		{
			m_dwStat = TRAN_STATE_ROLLBACK;
			//设置db的状态为初使状态
			if(m_pOper)
				m_pOper->SetState(DB_STATE_NONE);
		}
	}
	return dwRet;
}

