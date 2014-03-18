          
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <sstream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "svr_reactor_facade.h"
#include "tcp_handle_base.h"

#include <glog/logging.h>
#include <base/transport_pkg_head.h>

const  unsigned int __STX__ = 1179602771;
const uint32_t PKG_HEAD_LEN = sizeof(TRANSPORT_PKG_HEAD);
//版本信息
#define TCPBASE_VERSION "TCPBASE_VERSION 1.0."__DATE__   // version = major.minor.date            
const std::string tcpbase_nouse = TCPBASE_VERSION ; // 一定要此句,否则会被优化掉（不一定做全局量）

//////////////////////////////////
// class CTcpHandleBase
CTcpHandleBase::CTcpHandleBase(uint32_t dwMaxSendBufLen, uint32_t dwMaxRecvBufLen)
: m_pReactor(NULL)
, m_pSink(NULL)
, m_bAsServer(false)
, m_iConnHandle(-1)
, m_wPeerHostPort(0)
, m_nSendPos(0)
, m_nRecvPos(0)
, m_bNeedOnSend(false)
, m_l_nPacketLen(0)
, m_l_nCurrBufSendSize(0)
, m_l_nCurrBufRecvSize(0)
, m_l_nPos(0)
, m_cStatus(S_CLOSE)
, m_dwUserHandle(0)
{
	assert(dwMaxSendBufLen > 0);
	assert(dwMaxRecvBufLen > 0);

	m_achSendBuf = new char[dwMaxSendBufLen];
	m_achRecvBuf = new char[dwMaxRecvBufLen];

	assert(m_achSendBuf);
	assert(m_achRecvBuf);

	m_dwMaxSendBufLen = dwMaxSendBufLen;
	m_dwMaxRecvBufLen = dwMaxRecvBufLen;
}

CTcpHandleBase::~CTcpHandleBase()
{
	delete[] m_achSendBuf;
	delete[] m_achRecvBuf;
}

void CTcpHandleBase::Reset()
{
	m_iConnHandle = -1;
	m_wPeerHostPort = 0;
	m_nSendPos = 0;
	m_nRecvPos = 0;
	m_bNeedOnSend = false;
	m_l_nPacketLen = 0;
	m_l_nCurrBufSendSize = 0;
	m_l_nCurrBufRecvSize = 0;
	m_l_nPos = 0;
	m_cStatus = S_CLOSE;
	m_dwUserHandle = 0;
}

ITCPNetHandler* CTcpHandleBase::Clone() const
{
    CTcpHandleBase* pObj = new CTcpHandleBase(m_dwMaxSendBufLen, m_dwMaxRecvBufLen);
    pObj->m_pReactor = this->m_pReactor;
    pObj->m_pSink = this->m_pSink;
    pObj->m_bAsServer = this->m_bAsServer;
	return pObj;
}

int CTcpHandleBase::OnConnect(
                              int iConnHandle,
                              unsigned int uiPeerHost,
                              unsigned short ushPeerPort,
                              int iFlag)
{
    m_iConnHandle = iConnHandle;

	struct in_addr addr;
	addr.s_addr = uiPeerHost; // 网络字节序
    m_sPeerHostIP = inet_ntoa(addr);
	m_wPeerHostPort = ushPeerPort;
	m_dwPeerHostIP = uiPeerHost; // 网络字节序

    m_cStatus = S_CONNECTED;

    assert(m_pSink);
    m_pSink->OnConnect(this, m_sPeerHostIP.c_str(), ushPeerPort);
    
    return 0;
}

int CTcpHandleBase::OnRecv(
                           int iConnHandle,
                           char* pRecvBuf,
                           uint32_t nBufLen,
                           int iFlag)
{
    assert(m_iConnHandle == iConnHandle);

    m_l_nCurrBufRecvSize = m_nRecvPos + nBufLen;

	if(m_dwMaxRecvBufLen < m_l_nCurrBufRecvSize)
	{
		char buf[128] = {0};
		snprintf(buf, sizeof(buf), "m_nRecvPos[%u], nBufLen[%u], m_l_nCurrBufRecvSize[%u] > m_dwMaxRecvBufLen[%u] From %s:%d", m_nRecvPos, nBufLen, m_l_nCurrBufRecvSize, m_dwMaxRecvBufLen, this->GetPeerHostIP().c_str  (), this->GetPeerHostPort());

		// Throw Error to Uplayer!!!
        assert(m_pSink);
        m_pSink->OnRecv(
            this,  
            buf, 0);

		// Reset recv buffer
		m_l_nCurrBufRecvSize = 0;		
		m_nRecvPos = 0;

		return -1;
	}

    // Keep this piece
    memmove(m_achRecvBuf + m_nRecvPos, pRecvBuf, nBufLen);
    m_nRecvPos = m_l_nCurrBufRecvSize;
        
    if(m_l_nCurrBufRecvSize < PKG_HEAD_LEN)
    {
        //m_nRecvPos = m_l_nCurrBufRecvSize;
        return 0;
    }
    
    unsigned int wSTX = 0;
    m_l_nPos = 0;
    m_l_nPacketLen = 0;
    
    while(m_l_nCurrBufRecvSize >= PKG_HEAD_LEN)
    {
        unsigned int *pmagic = reinterpret_cast<unsigned int*>(m_achRecvBuf + m_l_nPos);
        wSTX = ntohl(*pmagic);
        
        if(__STX__ != wSTX)
        {
			char buf[128] = {0};
			snprintf(buf, sizeof(buf), "__STX__(0x%x) != wSTX(0x%x), From %s:%d", __STX__, wSTX, this->GetPeerHostIP().c_str(), this->GetPeerHostPort());

			// Throw Error to Uplayer!!!
			assert(m_pSink);
			m_pSink->OnRecv(
					this,  
             
       					buf, 0);
            
            pmagic += 1;
            m_l_nPos += sizeof(__STX__);
            m_l_nCurrBufRecvSize -= sizeof(__STX__);
            
            while(m_l_nCurrBufRecvSize >= PKG_HEAD_LEN)
            {
                  wSTX = ntohl(*pmagic);                    
 
		    if(__STX__ == wSTX)
                    break;

                pmagic += 1;
                m_l_nPos += sizeof(__STX__);
                m_l_nCurrBufRecvSize -= sizeof(__STX__);
            }
            
            continue;
        }
        
        PTRANSPORT_PKG_HEAD p_pkg = reinterpret_cast<PTRANSPORT_PKG_HEAD>(pmagic);
        
        m_l_nPacketLen = ntohl(p_pkg->length) + PKG_HEAD_LEN;
        if(m_l_nCurrBufRecvSize < m_l_nPacketLen)
        {
            // Need more buf to fill a packet
	    //		m_l_nPos -= PKG_HEAD_LEN;
	    //		m_l_nCurrBufRecvSize += PKG_HEAD_LEN;
			
            break;
        }
        
        assert(m_pSink);
        m_pSink->OnRecv(
            this,  
            m_achRecvBuf + m_l_nPos, m_l_nPacketLen);
        
        m_l_nPos += m_l_nPacketLen;
        m_l_nCurrBufRecvSize -= m_l_nPacketLen;
    }
    
    if(m_l_nCurrBufRecvSize == 0) // All buffer has been callbacked to upper layer
    {
        m_nRecvPos = 0;
    }
    else
    {
        memmove(m_achRecvBuf, m_achRecvBuf + m_l_nPos, m_l_nCurrBufRecvSize);
        m_nRecvPos = m_l_nCurrBufRecvSize;
    }
    
    return 0;
}

int CTcpHandleBase::OnSend(
						   int iConnHandle
						   )
{
    assert(m_iConnHandle == iConnHandle);
    
    if(m_bNeedOnSend)
    {
        assert(m_pSink);
        m_pSink->OnSend(this);
        
    }
    
    return 0;
}

int CTcpHandleBase::OnClose(
                            int iConnHandle,
                            int iCloseCode,
                            int iFlag)
{
    assert(m_iConnHandle == iConnHandle);
    assert(m_pSink);

    m_cStatus = S_CLOSE;
    
    m_pSink->OnClose(this);
    
    return 0;
}

int CTcpHandleBase::ConnectTCPServer(
                     const char* pszIP,
                     uint16_t ushPort,
		     uint32_t dwConnTimeout)
{
    assert(!m_bAsServer);
    assert(m_pReactor);
    return m_pReactor->ConnectToServer(
        this,
        ushPort, 
        pszIP,
        dwConnTimeout);
}

int CTcpHandleBase::ConnectUSockServer(
                       const char* pszUSockPath,
		       uint32_t dwConnTimeout)
{
    assert(!m_bAsServer);
    assert(m_pReactor);
    return m_pReactor->ConnectToUSockTCPServer(
        this,
        pszUSockPath,
        dwConnTimeout);
}

int CTcpHandleBase::SendData(
                             const char* pSendBuf,
                             uint32_t nBufLen
                             )
{
    if(m_cStatus == S_CLOSE || m_iConnHandle == -1)
    {
        return -2;   // Not ready
    }

	if(nBufLen > 0) 
	{
		if((m_nSendPos + nBufLen) > m_dwMaxSendBufLen)
		{
			m_bNeedOnSend = true;
                        
			LOG(ERROR)<<"TcpHandleBase::SendData fail, send buffer short"<<std::endl;
			//if(m_nSendPos > 0)
			//			SendData(); // OnSend Directly
			return -1;   // Too much data to send
		}
	}
	else
	{
		// OnSend Flush ... Just go through
	}

        m_l_nCurrBufSendSize = m_nSendPos + nBufLen;
	if(m_l_nCurrBufSendSize == 0)
	{
        return 0;
	}
        
        if (pSendBuf)
        {
            memmove(m_achSendBuf + m_nSendPos, pSendBuf, nBufLen);
	    m_nSendPos = m_l_nCurrBufSendSize;
        }
  
	/*if(pSendBuf)
	{
		// Keep this piece
		m_oByteStreamNet.resetStreamBuf(m_achSendBuf + m_nSendPos, m_dwMaxSendBufLen - m_nSendPos);
		m_oByteStreamNet << __STX__;
		m_oByteStreamNet << nBufLen;
		m_oByteStreamNet.Write(pSendBuf, nBufLen);

        m_l_nCurrBufSendSize += PKG_HEAD_LEN;
	}*/
    
    assert(m_pReactor);
    
    int szByteSent = m_pReactor->Send(
        m_iConnHandle,
        m_achSendBuf, 
        m_nSendPos);
    
	if(szByteSent < 0)	
	{
		return -3; // Peer must be closed
	}
	else if(szByteSent > 0)  // Bytes have been sent
    {
        if((uint32_t)szByteSent != m_nSendPos)
        {
            //printf("NeedOnSend, szByteSent[%d] != m_l_nCurrBufSendSize[%d]\n", szByteSent, m_l_nCurrBufSendSize);
            assert(m_nSendPos > (uint32_t)szByteSent);
            
            memmove(m_achSendBuf, m_achSendBuf + szByteSent, m_nSendPos - szByteSent);
            m_nSendPos -= szByteSent;
            
            m_bNeedOnSend = true;
            m_pReactor->NeedOnSendCheck(m_iConnHandle);
            
            return 0;
        }
        else    // szByteSent == m_l_nCurrBufSendSize
        {
    		m_bNeedOnSend = false;
            m_nSendPos = 0;
            return 0;
        }
    }

    //printf("NeedOnSend, szByteSent[%d] == 0\n", szByteSent);
    
    // No byte send
    //m_nSendPos = m_l_nCurrBufSendSize;
    m_bNeedOnSend = true;
    m_pReactor->NeedOnSendCheck(m_iConnHandle);

    return 0; // No byte send
}

int CTcpHandleBase::Close()
{
    if(m_cStatus == S_CLOSE)
        return 0;

	// If Some RecvBuf left, there must be something wrong	
	if(m_nRecvPos > 0)
	{
		// Dump(m_achRecvBuf, m_nRecvPos);
		std::stringstream oss;
		//c2cplatform::library::util::HEX_DUMP->DebugHexDump(oss, m_achRecvBuf, m_nRecvPos);
		// Throw Error to Uplayer!!!
        assert(m_pSink);
        m_pSink->OnRecv(
            this,  
            const_cast<char*>("Some Data there while closing"), 0);
        m_pSink->OnRecv(
            this,  
            const_cast<char*>(oss.str().c_str()), 0);
	}
	else
	{
		// Throw Error to Uplayer!!!
        assert(m_pSink);
        m_pSink->OnRecv(
            this,  
            const_cast<char*>("No Data Left while closing"), 0);
	}

    m_cStatus = S_CLOSE;

    assert(m_pReactor);
    int iRetCode = m_pReactor->Close(m_iConnHandle, m_bAsServer);
    this->Reset();
    return iRetCode;
}


