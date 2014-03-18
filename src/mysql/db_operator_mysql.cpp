#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <string>
#include <sstream>
#include <iostream>
#include <sys/time.h>
#include "db_operator_mysql.h"


CDBOperatorMysql::CDBOperatorMysql()
{
	memset(m_szHostAddress, 0, sizeof(m_szHostAddress));
	memset(m_szUserName, 0, sizeof(m_szUserName));
	memset(m_szPassword, 0, sizeof(m_szPassword));
	memset(m_szDBName, 0, sizeof(m_szDBName));
	memset(m_szDBCharSet, 0, sizeof(m_szDBCharSet));

	m_bConnected = false;
	m_bSqlLog    = false;
	m_nState = 0;
}

CDBOperatorMysql::~CDBOperatorMysql()
{	
	CloseDB();
}

long long CDBOperatorMysql::GetCurrTime_US(){
	struct timeval stTime;
	gettimeofday(&stTime,NULL);		
	long long tmpTime = stTime.tv_sec * 1000000;
	return tmpTime + stTime.tv_usec;
}

uint32_t CDBOperatorMysql::GetCurrTime_S(){
	struct timeval stTime;
	gettimeofday(&stTime,NULL);
	return stTime.tv_sec;
}


int CDBOperatorMysql::GetFieldIndex(const char* pszFieldName)
{
	assert(pszFieldName);
	if(NULL == m_stDBLink.pstRes)
	{
		return -1;
	}

	int iNumOfFields = mysql_num_fields(m_stDBLink.pstRes);
	MYSQL_FIELD* fields = mysql_fetch_fields(m_stDBLink.pstRes);
	for(int i = 0; i < iNumOfFields; i++)
	{
		if(NULL == fields[i].name)
			continue;
			
		if(strcasecmp(pszFieldName, fields[i].name) == 0)
			return i;
	}
	return -2;
}

IDBOperator* CDBOperatorMysql::clone()
{
	CDBOperatorMysql* pDBOpr = new CDBOperatorMysql;
	try {
		pDBOpr->InitDB(
				m_szHostAddress, 
				m_szUserName, 
				m_szPassword, 
				m_szDBName,
				m_szDBCharSet);
	} 
	catch(CDBOperatorMysql::CDbException& e)
	{
		printf("InitDB ERROR : %s\n", e.what());
		delete pDBOpr;
		return NULL;
	}

	return pDBOpr;
}

bool CDBOperatorMysql::ReConnectDB()
{
	int i = 0;
	
	while (!m_bConnected && (i++ < 3))		
	{
		m_bConnected = InitDB(
				m_szHostAddress, 
				m_szUserName, 
				m_szPassword, 
				m_szDBName,
				m_szDBCharSet);
		//sleep(1);
	}
	
	return m_bConnected;
}

bool CDBOperatorMysql::InitDB(
		const char* pszHostAddress,
		const char* pszUserName,
		const char* pszPassword,
		const char* pszDBName,
		const char* pszDBCharSet)
{
	int iRetCode = 0;

	
	if (m_bConnected)
		CloseDB();

	memset(&m_stDBLink, 0, sizeof(CDB_LINK));

	mysql_init(&(m_stDBLink.stMysqlConn.stMysql));
	
	m_stDBLink.iResNotNull = 0;
	m_stDBLink.stMysqlConn.iDBConnect  = 0;

	
	if (strcmp(m_szHostAddress, pszHostAddress)) snprintf(m_szHostAddress, sizeof(m_szHostAddress), "%s", pszHostAddress);
	if (strcmp(m_szUserName, pszUserName)) snprintf(m_szUserName, sizeof(m_szUserName), "%s", pszUserName);
	if (strcmp(m_szPassword, pszPassword)) snprintf(m_szPassword, sizeof(m_szPassword), "%s", pszPassword);
	if (strcmp(m_szDBName, pszDBName)) snprintf(m_szDBName, sizeof(m_szDBName), "%s", pszDBName);
	if (strcmp(m_szDBCharSet, pszDBCharSet)) snprintf(m_szDBCharSet, sizeof(m_szDBCharSet), "%s", pszDBCharSet);

	iRetCode = ConnectDB();

	if (iRetCode!= 0)
	{
		m_bConnected = false;
		std::stringstream oss;
		oss << "Connect to DB Failed! DBHost[" << m_szHostAddress << \
			"] DBUser[" << m_szUserName << "] DBName[" << m_szDBName << "] DBCharSet[" << m_szDBCharSet << "] ";
		std::string sErrMsg = oss.str();	
		sErrMsg += m_szErrMsg;
		throw CDbException(sErrMsg.c_str(), GetLastErrno());
	}
	else	
	{
		printf("Connected to  %s@%s\n", pszDBName, m_szHostAddress);
		std::stringstream oss;
		m_bConnected = true;
	}

	if(m_szDBCharSet[0] != '\0')
	{
		std::cout << "Connect to DB OK! DBHost[" << m_szHostAddress << \
			"] DBUser[" << m_szUserName << "] DBName[" << m_szDBName << "] DBCharSet[" << m_szDBCharSet << "]" << std::endl;

        SetCharSet(m_szDBCharSet);
	}
	else
	{
		std::cout << "Connect to DB OK! DBHost[" << m_szHostAddress << \
			"] DBUser[" << m_szUserName << "] DBName[" << m_szDBName << "]" << std::endl;
	}

	return m_bConnected;
}


int CDBOperatorMysql::ConnectDB()
{

	int iSelectDB = 0;
	
	//如果要连接的地址和当前的地址不是同一台机器则先close当前的连接,再重新建立连接
	if (strcmp(m_stDBLink.stMysqlConn.sHostAddress, m_szHostAddress) != 0)
	{
		if (m_stDBLink.stMysqlConn.iDBConnect==1)
		{
			CloseDB();
		}

		char szHostAddress[DB_MAX_CONF_STR_LEN] = {0};
		memcpy(szHostAddress, m_szHostAddress, DB_MAX_CONF_STR_LEN);
		
		uint32_t dwPort = 0;
		char *pTmp = strchr(szHostAddress, ':');
		if (pTmp != NULL)
		{
			*pTmp = 0;
			dwPort = strtoul(pTmp + 1, NULL, 10);
		}
		
		if (mysql_real_connect(&(m_stDBLink.stMysqlConn.stMysql), 
			szHostAddress, m_szUserName, m_szPassword, m_szDBName, dwPort, NULL,0) == 0)
		{
			m_stDBLink.stMysqlConn.iMySQLErrno = mysql_errno(&(m_stDBLink.stMysqlConn.stMysql));
			snprintf(m_szErrMsg, sizeof(m_szErrMsg), 
				"Fail To Connect To Mysql: %s", mysql_error(&(m_stDBLink.stMysqlConn.stMysql)));
			return -1;
		}	
		m_stDBLink.stMysqlConn.iDBConnect=1;
		iSelectDB = 1;
	}
	else 
	{
		if (m_stDBLink.stMysqlConn.iDBConnect==0){
			if (mysql_real_connect(&(m_stDBLink.stMysqlConn.stMysql), 
				m_szHostAddress, m_szUserName, m_szPassword, m_szDBName, 0, NULL, 0) == 0)
			{
				m_stDBLink.stMysqlConn.iMySQLErrno = mysql_errno(&(m_stDBLink.stMysqlConn.stMysql));
				snprintf(m_szErrMsg, sizeof(m_szErrMsg), 
					"Fail To Connect To Mysql: %s", mysql_error(&(m_stDBLink.stMysqlConn.stMysql)));
				return -1;
			}	
			m_stDBLink.stMysqlConn.iDBConnect=1;
		}
		else{
			if(mysql_ping(&(m_stDBLink.stMysqlConn.stMysql)) != 0)
			{
				m_stDBLink.stMysqlConn.iMySQLErrno = mysql_errno(&(m_stDBLink.stMysqlConn.stMysql));
				snprintf(m_szErrMsg, sizeof(m_szErrMsg), 
					"Fail To ping To Mysql: %s", mysql_error(&(m_stDBLink.stMysqlConn.stMysql)));
				return -1;
			}
		}	
	}	

	if ((iSelectDB != 0) || (strcmp(m_stDBLink.stMysqlConn.sDBName, m_szDBName) != 0))
	{
		if (mysql_select_db(&(m_stDBLink.stMysqlConn.stMysql), m_szDBName) < 0)
		{
			m_stDBLink.stMysqlConn.iMySQLErrno = mysql_errno(&(m_stDBLink.stMysqlConn.stMysql));
			snprintf(m_szErrMsg, sizeof(m_szErrMsg), 
				"Cannot Select Database %s: %s", m_szDBName, mysql_error(&(m_stDBLink.stMysqlConn.stMysql)));
			return -1;
		}
	}	

	
	strncpy(m_stDBLink.stMysqlConn.sHostAddress, m_szHostAddress, sizeof(m_stDBLink.stMysqlConn.sHostAddress) - 1);
	strncpy(m_stDBLink.stMysqlConn.sDBName, m_szDBName, sizeof(m_stDBLink.stMysqlConn.sDBName) - 1);
	strncpy(m_stDBLink.stMysqlConn.sUserName, m_szUserName, sizeof(m_stDBLink.stMysqlConn.sUserName) - 1);

	ResetBinLogName();


	m_stDBLink.dwLastExecSqlTime = GetCurrTime_S();
	return 0;
}

void CDBOperatorMysql::ResetBinLogName()
{
	m_sBinLogName.resize(0);
	
	m_sBinLogName += m_stDBLink.stMysqlConn.sDBName;
	m_sBinLogName += "_";
	m_sBinLogName += m_stDBLink.stMysqlConn.sHostAddress;
	m_sBinLogName += "_";
	m_sBinLogName += m_stDBLink.stMysqlConn.sUserName;
	m_sBinLogName += "_";
	m_sBinLogName += "pl_binlog";
}

bool CDBOperatorMysql::CloseDB()
{
	if (m_stDBLink.iResNotNull == 1)
	{
		mysql_free_result(m_stDBLink.pstRes);
		m_stDBLink.iResNotNull=0;
	}		
	
	if (m_stDBLink.stMysqlConn.iDBConnect == 1)
		mysql_close(&(m_stDBLink.stMysqlConn.stMysql));
	
	
	m_stDBLink.stMysqlConn.iDBConnect = 0;
	m_stDBLink.stMysqlConn.sHostAddress[0] = 0;
	m_stDBLink.dwLastExecSqlTime = 0;
	m_bConnected = false;
	m_nState = 0;
	
	return true;	
}

void CDBOperatorMysql::CheckLastExecTimeForReConnect()
{
	if(m_bConnected)
	{
		uint32_t dwNowTime = GetCurrTime_S();
		if((dwNowTime-m_stDBLink.dwLastExecSqlTime)>MYSQL_AUTORECONNECT_TIME)
		{
			if(m_bSqlLog)
			{
				printf("pid[%d] CheckLastExecTimeForReConnect Reason[dwNowTime:%u][dwLastExecSqlTime:%u][dwNowTime-dwLastExecSqlTime:%u]", getpid() ,dwNowTime,m_stDBLink.dwLastExecSqlTime,dwNowTime-m_stDBLink.dwLastExecSqlTime);
			}

			CloseDB();
			ReConnectDB();
		}
		m_stDBLink.dwLastExecSqlTime = dwNowTime;
	}
}
	
void CDBOperatorMysql::ExecSQL(const char * strSQL)
{
	//1、判断是否已经建立连接，如果没有建立链接，则建立链接。
	//   如果建立失败，则直接向上抛出异常,不会从住下走了。
	if (!m_bConnected)
	{
		ReConnectDB();
		
		//如果走到这里说明之前已经出错了。引起重新连接。
		//if(m_bSqlLog)
		//printf("pid[%d] ReConnectDB m_nState=[%d] [ ExecSQL m_bConnected=false] sql=[%s]", getpid(), m_nState, strSQL);
	}

	//2、判断这个链接是否已经600秒没有操作，如果是，则重新建立连接
	//   如果此时的连接不是一个事务，就会去检查是否是超时
	if(m_nState != DB_TRAN_STATE_FLAG)
		CheckLastExecTimeForReConnect();
	
	
	//3、组装sql语句
	m_stDBLink.iQueryType=0;
	if(snprintf(m_stDBLink.sQuery, sizeof(m_stDBLink.sQuery), "%s", strSQL)  !=  (int)strlen(strSQL))
	{
		snprintf(m_szErrMsg, sizeof(m_szErrMsg), "SQL is not valid sql=[%s]", strSQL);
		
		printf("pid[%d] snprintf error m_nState=[%d] sql=[%s]", getpid(), m_nState, strSQL);
		
		//这里先简单处理，只要失败，就关闭连接
		if(m_nState != DB_TRAN_STATE_FLAG)
		{
			CloseDB();
		}
		//先临时用这个错误码9999999
		throw CDbException(m_szErrMsg, 9999999);
	}
	
	//4、执行sql
	int iRetCode = ExecSQL_i();
	if(iRetCode != 0)
	{
		//4.1 如果有失败，这里记个log观察
		//if(m_bSqlLog)
		printf("pid[%d]  ExecSQL error m_nState=[%d] sql=[%s][iRetCode:%d][iDBConnect:%d] [ErrorMsg:%s]", getpid(), m_nState, strSQL, iRetCode,m_stDBLink.stMysqlConn.iDBConnect, m_szErrMsg);
	
		//如果此次是事务操作时，直接抛出异常
		if(m_nState == DB_TRAN_STATE_FLAG)
		{
			throw CDbException(m_szErrMsg, GetLastErrno());
		}
		
		CloseDB();
		
		if((m_stDBLink.stMysqlConn.iDBConnect == 0) )
		{
			ReConnectDB();
			m_stDBLink.iQueryType=0;	
			if(snprintf(m_stDBLink.sQuery, sizeof(m_stDBLink.sQuery), "%s", strSQL) !=  (int)strlen(strSQL))
			{
				CloseDB();
				throw CDbException(m_szErrMsg, GetLastErrno());
			}
			iRetCode = ExecSQL_i();
			if(iRetCode != 0)
			{
				CloseDB();
				throw CDbException(m_szErrMsg, GetLastErrno());
			}
		}
	}			
}

void CDBOperatorMysql::SetSqlLog(bool bSwitcher)
{
	m_bSqlLog = bSwitcher;
}

int CDBOperatorMysql::ExecSQL_i() 
{
	//由于数据库版本的问题，这里无奈加这行判断
	if (mysql_ping(&(m_stDBLink.stMysqlConn.stMysql)))
	{
		ReConnectDB();
		
		//如果走到这里说明之前已经出错了。引起重新连接。
		//if(m_bSqlLog)
		//printf("pid[%d] ReConnectDB m_nState=[%d] [ ExecSQL m_bConnected=false] sql=[%s]", getpid(), m_nState, strSQL);
	}
		
	int   iRetCode = 0;
	
	// 检查参数是否正确
	if (m_stDBLink.iQueryType != 0)
	{
		if ((m_stDBLink.sQuery[0]!='s') && (m_stDBLink.sQuery[0]!='S'))
		{
			snprintf(m_szErrMsg, sizeof(m_szErrMsg), "QueryType=1, But SQL is not select");
			return -1;
		}
	}
	// 是否需要关闭原来RecordSet
	if (m_stDBLink.iResNotNull==1)
	{
		mysql_free_result(m_stDBLink.pstRes);
		m_stDBLink.iResNotNull=0;
	}		
	if (m_stDBLink.stMysqlConn.iDBConnect == 0)
	{
		snprintf(m_szErrMsg, sizeof(m_szErrMsg), "Has Not Connect To DB Server Yet");
		return -1;
	}

	uint64_t ddwStartUs = 0;
	if(m_bSqlLog)
	{
		ddwStartUs = GetCurrTime_US();
	}
	
	// 执行相应的SQL语句
	iRetCode =mysql_query(&(m_stDBLink.stMysqlConn.stMysql), m_stDBLink.sQuery);
	if (iRetCode != 0)
	{
		//modify by wendyhu 对于异常情况，直接把错误返回给上层。
		/**
		m_stDBLink.stMysqlConn.iMySQLErrno = mysql_errno(&(m_stDBLink.stMysqlConn.stMysql));
		int iMySqlErrno = m_stDBLink.stMysqlConn.iMySQLErrno;
		
		snprintf(m_szErrMsg, sizeof(m_szErrMsg), "Fail To Execute SQL: [%d][%s][%s]", 
			m_stDBLink.stMysqlConn.iMySQLErrno,
			mysql_error(&(m_stDBLink.stMysqlConn.stMysql)), 
			m_stDBLink.sQuery);
		if((1046<=iMySqlErrno<=1052)
				|| (1054<=iMySqlErrno<=1075)
				|| (1099<=iMySqlErrno<=1104)
				|| (1109<=iMySqlErrno<=1119)
				|| (iMySqlErrno==1146)
				|| (1148<=iMySqlErrno<=1149)
				|| (1179<=iMySqlErrno<=1182)
				|| (1205<=iMySqlErrno<=1209)
				|| (1212<=iMySqlErrno<=1217)
				|| (1220<=iMySqlErrno<=1224)
				|| (iMySqlErrno==1412)
		)
		{
			CloseDB();
			throw CDbException(m_szErrMsg, GetLastErrno());
		}*/
		
		return iRetCode;
	}
	if(m_bSqlLog)
	{
		
		uint64_t ddwUseTime = (GetCurrTime_US() - ddwStartUs);
		// Try to Make PLBinLog
		printf("pid[%d]DBHost[%s]DBName[%s]DBUser[%s]DBConnOn[%d]ResNotNull[%d]ResNum[%d]QueryType[%d]UseTime[%lld]SQL[USE %s;%s;]", 
				getpid(),
				m_stDBLink.stMysqlConn.sHostAddress,
				m_stDBLink.stMysqlConn.sDBName,
				m_stDBLink.stMysqlConn.sUserName,
				m_stDBLink.stMysqlConn.iDBConnect,
				m_stDBLink.iResNotNull,
				m_stDBLink.iResNum,
				m_stDBLink.iQueryType,
				ddwUseTime,
				m_stDBLink.stMysqlConn.sDBName,
				m_stDBLink.sQuery
				);
	}
	// 保存结果
	if (m_stDBLink.iQueryType == 1)
	{
		m_stDBLink.pstRes = mysql_store_result(&(m_stDBLink.stMysqlConn.stMysql));
		//add by anderszhou 2007-09-18
		//因为 mysql_store_result 可能在以下两种情况下返回 Null, 所以在下面增加对null值的判断和处理
		// 1. 内存不够
		// 2. 字段数量为零
		if(m_stDBLink.pstRes == NULL)
		{
			m_stDBLink.iResNum = 0;
			m_stDBLink.iResNotNull = 0;
		}
		else
		{ 
			m_stDBLink.iResNum = mysql_num_rows(m_stDBLink.pstRes);
			m_stDBLink.iResNotNull = 1;
		}
	}

	return 0;
}


void CDBOperatorMysql::Query(const char * strSQL)
{
	if (!m_bConnected)
	{
		ReConnectDB();

		if(m_bSqlLog)
			printf("pid[%d] ReConnectDB Reason[ Query m_bConnected=false]", getpid() );
	}

	if(m_nState != DB_TRAN_STATE_FLAG)
		CheckLastExecTimeForReConnect();

	m_stDBLink.iQueryType=1;	
	if (snprintf(m_stDBLink.sQuery, sizeof(m_stDBLink.sQuery), "%s", strSQL) !=  (int)strlen(strSQL))
	{
		snprintf(m_szErrMsg, sizeof(m_szErrMsg), "SQL is not valid sql=[%s]", strSQL);
		
		printf("pid[%d] snprintf error m_nState=[%d] sql=[%s]", getpid(), m_nState, strSQL);
		
		//这里先简单处理，只要失败，就关闭连接
		if(m_nState != DB_TRAN_STATE_FLAG)
		{
			CloseDB();
		}
		//先临时用这个错误码9999999
		throw CDbException(m_szErrMsg, 9999999);
	}
	
	int iRetCode = ExecSQL_i ();
	if (iRetCode != 0)
	{
		if(m_bSqlLog)
			printf("pid[%d] ReConnectDB Reason[ Query CloseDB][iRetCode:%d][iDBConnect:%d] [ErrorMsg:%s]", getpid(),iRetCode,m_stDBLink.stMysqlConn.iDBConnect,m_szErrMsg);
	
		//如果此次是事务操作时，直接抛出异常
		if(m_nState == DB_TRAN_STATE_FLAG)
		{
			throw CDbException(m_szErrMsg, GetLastErrno());
		}
		
		CloseDB();
		
		if( (m_stDBLink.stMysqlConn.iDBConnect == 0))
		{
			ReConnectDB();
			m_stDBLink.iQueryType=1;	
			if (snprintf(m_stDBLink.sQuery, sizeof(m_stDBLink.sQuery), "%s", strSQL) !=  (int)strlen(strSQL))
			{
				CloseDB();
				throw CDbException(m_szErrMsg, GetLastErrno());
			}
			iRetCode = ExecSQL_i();
			if(iRetCode != 0)
			{
				CloseDB();
				throw CDbException(m_szErrMsg, GetLastErrno());
			}
		}
	}	
}

int CDBOperatorMysql::SetCharSet(const char * szCharSet)
{
	char szSql[100] = {0};
   	 snprintf(szSql, sizeof(szSql), "SET NAMES '%s'", szCharSet);

	if (!m_bConnected)
	{
		ReConnectDB();

		if(m_bSqlLog)
			printf("pid[%d] ReConnectDB Reason[ SetCharSet m_bConnected=false]", getpid() );
	}

	m_stDBLink.iQueryType=1;	
	if (snprintf(m_stDBLink.sQuery, sizeof(m_stDBLink.sQuery), "%s", szSql) == -1)
	{
		CloseDB();
		throw CDbException(m_szErrMsg, GetLastErrno());
	}
	
	int iRetCode = ExecSQL_i ();
	if (iRetCode != 0)
	{
		std::cout << "Invalid CharSet " << szCharSet << ", DB connection will be close.\n";
		CloseDB();
		throw CDbException(m_szErrMsg, GetLastErrno());
	}	
	return 0;
}


int	CDBOperatorMysql::GetRowNum()
{
	return m_stDBLink.iResNum;	
}

int	CDBOperatorMysql::GetFieldNum()
{
	return mysql_num_fields(m_stDBLink.pstRes);
}

const MYSQL_ROW& CDBOperatorMysql::FetchRow()
{	
	if (m_stDBLink.iResNotNull == 0)
	{
		CloseDB();
		throw CDbException("Recordset is Null", GetLastErrno());
	}		
	
	if (m_stDBLink.iResNum == 0)
	{
		CloseDB();
		throw CDbException("Recordset count=0", GetLastErrno());	
	}		

	return (m_stDBLink.stRow = mysql_fetch_row(m_stDBLink.pstRes));
}

void CDBOperatorMysql::FreeQueryResult()
{
	if (m_stDBLink.iResNotNull==1)
	{
		mysql_free_result(m_stDBLink.pstRes);
		m_stDBLink.iResNotNull=0;
	}	
}

uint32_t CDBOperatorMysql::GetInertId()
{
	return mysql_insert_id(&(m_stDBLink.stMysqlConn.stMysql));
}

std::string CDBOperatorMysql::GetDBHost()
{
	return m_szHostAddress;
}

std::string CDBOperatorMysql::GetDBName() 
{
	return m_szDBName;		
}
std::string CDBOperatorMysql::GetDBUser() 
{
	return m_szUserName;
}
std::string CDBOperatorMysql::GetDBCharset()
{
	return m_szDBCharSet;
}

void CDBOperatorMysql::EscapeString(char * sTo, uint32_t dwToLen, const char * sFrom, uint32_t dwFromLen)
{
	if(0 == strcasecmp("GBK", m_szDBCharSet)) 
	{
		CleanChineseString(sFrom, dwFromLen, sTo, dwToLen);
		return;
	}

	if (RealEscapeString(sFrom, dwFromLen) == -1)
	{
		throw CDbException(m_szErrMsg, GetLastErrno());
	}
}

char*  CDBOperatorMysql::EscapeString(const char * sFrom, uint32_t dwFromLen)
{
	if(0 == strcasecmp("GBK", m_szDBCharSet)) 
	{
		CleanChineseString(sFrom, dwFromLen, m_szEscapedString, sizeof(m_szEscapedString));
		return m_szEscapedString;
	}
       
	if (RealEscapeString(sFrom, dwFromLen) == -1)
	{
		throw CDbException(m_szErrMsg, GetLastErrno());
	}
	return m_szEscapedString;
}


int CDBOperatorMysql::RealEscapeString(const char * sFrom, unsigned long ulFromLen)
{
	if (!sFrom || (unsigned long)MAX_QUERY_STR_LEN <= 2 * ulFromLen)
	{
		snprintf(m_szErrMsg, sizeof(m_szErrMsg), "invalid input");
		return -1;
	}
	
	return mysql_real_escape_string(&(m_stDBLink.stMysqlConn.stMysql), m_szEscapedString,
		sFrom, ulFromLen);
}


int CDBOperatorMysql::GetLastErrno()
{
	return m_stDBLink.stMysqlConn.iMySQLErrno;
}


char*  CDBOperatorMysql::EscapeString(const char * sFrom)
{
	uint32_t dwFromLen = strlen(sFrom);       
	return EscapeString(sFrom, dwFromLen);
}

char*  CDBOperatorMysql::EscapeString(const std::string& sFrom)
{
	return EscapeString(sFrom.c_str(), sFrom.length());
}

int CDBOperatorMysql::GetAffectedRows()
{
	if ( (m_stDBLink.stMysqlConn.iDBConnect == 0))
	{
		snprintf(m_szErrMsg, sizeof(m_szErrMsg), "Has Not Connect To DB Server Yet");
		return -1;
	}

	return mysql_affected_rows(&(m_stDBLink.stMysqlConn.stMysql));
	
}

bool CDBOperatorMysql::ISGBKLead(uint8_t c)
{
	return(c>0x80 && c<0xff);
}
bool CDBOperatorMysql::ISGBKNext(uint8_t c)
{
	return((c>0x3f && c<0x7f)||(c>0x7f && c<0xff));
}

// 去掉非法字符和半个汉字
void CDBOperatorMysql::CleanChineseString(const char *pszFrom, uint32_t dwFromLen, char* pszTo, uint32_t dwToLen)
{
	const char flag1 = '"';
	const char flag2[2 + 1] = "＂";
	const char flag3 = '\\';
	const char flag4[2 + 1] = "、";
	const char flag5 = '\'';
	const char flag6[2 + 1] = "’";

	uint32_t i,j;
	for (i = 0, j = 0; (i < dwFromLen && j < dwToLen); i++, j++)
	{
		/* 连续两个字符组成一个正确汉字*/
		if (ISGBKLead(pszFrom[i]))
		{
			/* 最后一个字符是非ascii, 半个汉字？ */
			if (i == dwFromLen - 1)
				break;

			if (ISGBKNext(pszFrom[i+1]))
			{
				pszTo[j] = pszFrom[i];
				i++;
				j++;
			}
			else /* 半个汉字？ */
			{
				pszTo[j] =' ';
				continue;
			}
		}
		else if (!isprint(pszFrom[i])) /* 无法识别的字符 */
		{
			pszTo[j] =' ';
			continue;
		}
		else
		{
			if(flag1 == pszFrom[i])
			{
				pszTo[j] = flag2[0];
				pszTo[j + 1] = flag2[1];
				j++;
				continue;
			}
			if(flag3 == pszFrom[i])
			{
				pszTo[j] = flag4[0];
				pszTo[j + 1] = flag4[1];
				j++;
				continue;
			}
			if(flag5 == pszFrom[i])
			{
				pszTo[j] = flag6[0];
				pszTo[j + 1] = flag6[1];
				j++;
				continue;
			}
		}
		pszTo[j] = pszFrom[i];
	}
	pszTo[j] = 0;
}


