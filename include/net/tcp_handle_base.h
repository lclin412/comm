 
#ifndef TCP_HANDLE_BASE_H
#define TCP_HANDLE_BASE_H

#include <stdint.h>
#include <string>
//#include "byte_stream.h"
#include "reactor_handler.h"

#ifdef __IM_SERVER_CONFIG__
const uint32_t MAX_TCP_HANDLE_BUF_LEN = 10 * 1024;
#else
const uint32_t MAX_TCP_HANDLE_BUF_LEN = 1024 * 1024;
#endif

class CReactor;
class CTcpHandleBase;

class ITCPPacketHandleSink
{
public:
    virtual ~ITCPPacketHandleSink() {};

public:
    virtual int OnConnect(
        CTcpHandleBase* pTcpHandleBase,
        const char* pszPeerIP,
        unsigned short ushPeerPort) = 0;

	/*
	 *	Recv a complete PDU
	 */
    virtual int OnRecv(
        CTcpHandleBase* pTcpHandleBase,
        char* pBuffer,
        uint32_t nBufLen) = 0;

	virtual int OnSend(
		CTcpHandleBase* pTcpHandleBase) = 0;

    virtual int OnClose(
        CTcpHandleBase* pTcpHandleBase) = 0;
};

class CTcpHandleBase : public ITCPNetHandler
{
protected:
    enum
    {
        S_CLOSE = 0,
        S_CONNECTED,
    };

public:
    CTcpHandleBase(uint32_t dwMaxSendBufLen = MAX_TCP_HANDLE_BUF_LEN, uint32_t dwMaxRecvBufLen = MAX_TCP_HANDLE_BUF_LEN);
    virtual ~CTcpHandleBase();
    
    virtual ITCPNetHandler* Clone() const;
    virtual void Reset();

public:

    int Initialize(
        CReactor* pReactor,
        ITCPPacketHandleSink* pSink,
        bool bAsServer)
    {
        m_pReactor = pReactor;
        m_pSink = pSink;
        m_bAsServer = bAsServer;
        m_cStatus = S_CLOSE;

        return 0;
    }

    bool IsConnected() const { return (m_cStatus == S_CONNECTED); }

    virtual int OnConnect(
        int iConnHandle,
        unsigned int uiPeerHost,
        unsigned short ushPeerPort,
        int iFlag);

    virtual int OnRecv(
        int iConnHandle,
        char* pRecvBuf,
        uint32_t nBufLen,
        int iFlag);

	virtual int OnSend(
		int iConnHandle
		);

    virtual int OnClose(
        int iConnHandle,
        int iCloseCode,
        int iFlag);

	/*
	 *	Send Data Block (Recommend nBufLen < 65532)
	 *  Return: 0 - Send OK; 
	 *         -1 - Send failed, no bytes sent
	 */
    virtual int SendData(
        const char* pSendBuf = NULL,
        uint32_t nBufLen = 0
        );

    virtual int Close();

public:
    int ConnectTCPServer(
        const char* pszIP,
        uint16_t ushPort,
	uint32_t dwConnTimeout = 0);

    int ConnectUSockServer(
        const char* pszUSockPath,
	uint32_t dwConnTimeout = 0);

    const std::string& GetPeerHostIP() const
    {
        return m_sPeerHostIP;
    }

    uint32_t GetDwPeerHostIP() const
    {
        return m_dwPeerHostIP;
    }

	uint16_t GetPeerHostPort() const
    {
        return m_wPeerHostPort;
    }

    int GetConnHandle() const
    {
        return m_iConnHandle;
    }

    CReactor* GetReactor()
    {
        return m_pReactor;
    }

	/*
	 *	Set User Defined Handle
	 */
	void SetUserHandle(uint32_t dwUserHandle)
	{
		m_dwUserHandle = dwUserHandle;
	}

	uint32_t GetUserHandle() const
	{
		return m_dwUserHandle;
	}


protected:
    CReactor* m_pReactor;
    ITCPPacketHandleSink* m_pSink;
    bool m_bAsServer;
    int m_iConnHandle;
	uint32_t m_dwPeerHostPort;
    std::string m_sPeerHostIP;
	uint16_t m_wPeerHostPort;

    char* m_achSendBuf;
    char* m_achRecvBuf;
	uint32_t m_dwMaxSendBufLen;
	uint32_t m_dwMaxRecvBufLen;

    uint32_t m_nSendPos;   // Position of last avail byte in SendBuf
    uint32_t m_nRecvPos;   // Position of last avail byte position in RecvBuf

    bool m_bNeedOnSend;
    int m_nReSendTimes;

    uint32_t m_l_nPacketLen;
    uint32_t m_l_nCurrBufSendSize;
    uint32_t m_l_nCurrBufRecvSize;
    uint32_t m_l_nPos;

    uint8_t m_cStatus;

	uint32_t m_dwUserHandle;

	uint32_t m_dwPeerHostIP;
};

#endif /* TCP_HANDLE_BASE_H */

