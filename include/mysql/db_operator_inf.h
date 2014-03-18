#ifndef DB_OPERATOR2_INTERFACE
#define  DB_OPERATOR2_INTERFACE

#include <sys/types.h>
#include <stdint.h>
#include <string>


typedef char** DB_ROW;

const int  DB_MAX_CONF_STR_LEN = 256;
const int  MAX_QUERY_STR_LEN = 80000;       //comform to size of code in TLIB
const int  DB_STATE_NONE = 0;               //db��ʶ
const int  DB_TRAN_STATE_FLAG = 1;          //dbΪ����״̬
const uint32_t  MYSQL_AUTORECONNECT_TIME = 600; //��λ��(s)


class  IDBOperator
{
public:
	struct CDbException{
		CDbException(const char* pszErrMsg, int iErrno = 0) : m_sErrMsg(pszErrMsg), m_iErrno(iErrno) {}
		const char* what() const { return m_sErrMsg.c_str(); }
		int mysql_errno() { return m_iErrno; }
		std::string m_sErrMsg;
		int m_iErrno;
	};

	virtual ~IDBOperator() {};
	virtual void SetSqlLog(bool bSwitcher) {};
	/**
	 *
	 * ��������һЩdb��״̬���������
	 * �����״ֵ̬����ʵ������ӿڵ���Ϊ����
	 *
	 * @param nState:״ֵ̬
	 *
	 */
	virtual void SetState(int nState) {};
	virtual IDBOperator* clone() = 0;
	virtual char* EscapeString(const std::string& sFrom)  = 0;
	virtual void ExecSQL(const char * strSQL)  = 0;
	virtual void Query(const char * strSQL)  = 0;
	
	virtual int	GetRowNum() = 0;
	virtual int	GetFieldNum() = 0;
	virtual const DB_ROW& FetchRow() = 0;
	virtual uint32_t GetInertId() = 0;
	virtual void FreeQueryResult() = 0;
	
	virtual const char * GetErrMsg() = 0;
	virtual int GetAffectedRows() = 0;
	virtual int GetFieldIndex(const char* pszFieldName) = 0;

	virtual bool CloseDB() = 0;
	
};


#endif

