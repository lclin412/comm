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
	char			sHostAddress[50];	//DB Server �ĵ�ַ
	char			sUserName[50];		//�û���
	char			sPassword[50];		//����
	char			sDBName[50];		//Database ����
	
	MYSQL			stMysql;		//��ǰ�򿪵�Mysql����
	
	int			iDBConnect;		//�Ƿ��Ѿ������϶�Ӧ��Database, 0=�Ͽ���1=������	
	int         iMySQLErrno;    //MySQL�Ĵ�����
};

class CDB_LINK
{
public:	
	CMYSQL_CONN		stMysqlConn; //Mysql��������ĵ�һ��

	MYSQL_RES		*pstRes; 	//��ǰ������RecordSet
	MYSQL_ROW		stRow;		//��ǰ������һ��
	  	
	int			iResNotNull;		//��ǰ������RecordSet�Ƿ�Ϊ��,0=�գ�1=�ǿ�
	int			iResNum;		//��ǰ������RecordSet�ļ�¼��Ŀ
	char		sQuery[80000];		//��ǰ������SQL���
	int			iQueryType;		//��ǰ������SQL����Ƿ񷵻�Recordset, 0=��Ҫ��1=select
	int			iMultiDBConn;		//�Ƿ� 0=ֻ��һ��mysql���ӣ�1=���mysql����
	uint32_t 		dwLastExecSqlTime;	//���ִ��SQLʱ��
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
	 * ��������һЩdb��״̬���������
	 * �����״ֵ̬����ʵ������ӿڵ���Ϊ����
	 *
	 * @param nState:״ֵ̬
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

