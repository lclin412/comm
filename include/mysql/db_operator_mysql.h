#ifndef DB_OPERATOR3_H
#define  DB_OPERATOR3_H

#include <sys/types.h>
#include <stdint.h>
#include <string>
#include "db_operator_inf.h"

extern "C" 
{
	#include "mysql.h"
}


class CMYSQL_CONN
{
public:	
	char			sHostAddress[50];	//DB Server 的地址
	char			sUserName[50];		//用户名
	char			sPassword[50];		//密码
	char			sDBName[50];		//Database 名字
	
	MYSQL			stMysql;		//当前打开的Mysql连接
	
	int			iDBConnect;		//是否已经连接上对应的Database, 0=断开，1=连接上	
	int         iMySQLErrno;    //MySQL的错误码
};

class CDB_LINK
{
public:	
	CMYSQL_CONN		stMysqlConn; //Mysql连接链表的第一项

	MYSQL_RES		*pstRes; 	//当前操作的RecordSet
	MYSQL_ROW		stRow;		//当前操作的一行
	  	
	int			iResNotNull;		//当前操作的RecordSet是否为空,0=空，1=非空
	int			iResNum;		//当前操作的RecordSet的记录数目
	char		sQuery[80000];		//当前操作的SQL语句
	int			iQueryType;		//当前操作的SQL语句是否返回Recordset, 0=不要，1=select
	int			iMultiDBConn;		//是否 0=只有一个mysql连接，1=多个mysql连接
	uint32_t 		dwLastExecSqlTime;	//最后执行SQL时间
};

class CDBOperatorMysql:public IDBOperator
{
public:
	CDBOperatorMysql();
	virtual ~CDBOperatorMysql();
	virtual IDBOperator* clone();
	virtual char* EscapeString(const std::string& sFrom);
	virtual void ExecSQL(const char * strSQL);
	virtual void Query(const char * strSQL);
	
	virtual int	GetRowNum();
	virtual int	GetFieldNum();
	virtual const MYSQL_ROW & FetchRow();
	virtual uint32_t GetInertId();
	
	virtual void FreeQueryResult();	
	virtual const char * GetErrMsg() { return m_szErrMsg; }
	virtual int GetAffectedRows();
	virtual int GetFieldIndex(const char* pszFieldName);
	
	bool InitDB(
			const char* pszHostAddress, 
			const char* pszUserName, 
			const char* pszPassword, 
			const char* pszDBName,
			const char* pszDBCharSet = "");
	
	bool CloseDB();

	std::string GetDBHost() ;
	std::string GetDBName() ;
	std::string GetDBUser() ;
	std::string GetDBCharset();

	void EscapeString(char * sTo, uint32_t dwToLen, const char * sFrom, uint32_t dwFromLen);
	char* EscapeString(const char * sFrom, uint32_t dwFromLen);
	char* EscapeString(const char * sFrom);
	

	virtual void SetSqlLog(bool bSwitcher);
	virtual bool IsSqlLog() {return m_bSqlLog;}
	
	/**
	 *
	 * 用来设置一些db的状态，如事务等
	 * 具体的状态值，由实现这个接口的类为定义
	 *
	 * @param nState:状态值
	 *
	 */
	virtual void SetState(int nState)
	{
		m_nState = nState;
	}
private:
	bool ISGBKLead(uint8_t c);
	bool ISGBKNext(uint8_t c);
	void CleanChineseString(const char *pszFrom, uint32_t dwFromLen, char* pszTo, uint32_t dwToLen);
	int ConnectDB();
	int ExecSQL_i();
	int RealEscapeString(const char * sFrom, unsigned long ulFromLen);
	int GetLastErrno();
	bool ReConnectDB();
	void ResetBinLogName();
	int SetCharSet(const char * strSQL);
	void CheckLastExecTimeForReConnect();
private:
	long long GetCurrTime_US();
	uint32_t GetCurrTime_S();
private:
	CDB_LINK m_stDBLink;	

	char m_szHostAddress[DB_MAX_CONF_STR_LEN];
	char m_szUserName[DB_MAX_CONF_STR_LEN];
	char m_szPassword[DB_MAX_CONF_STR_LEN];
	char m_szDBName[DB_MAX_CONF_STR_LEN];		
	char m_szDBCharSet[DB_MAX_CONF_STR_LEN];	

	bool m_bConnected;
	char m_szErrMsg[256];
	char m_szEscapedString[MAX_QUERY_STR_LEN];
	bool m_bSqlLog;
	int m_nState;
	std::string m_sBinLogName;
};


#endif

