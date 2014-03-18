
#include <errno.h>
#include <assert.h>
#include <unistd.h>
#include <fcntl.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <iostream>
#include <string>
#include <sstream>
#include "net_svr_reactor.h"
#include <errno.h>

const long DEFAULT_SELECT_TIMEOUT_SEC = 0;
const long DEFAULT_SELECT_TIMEOUT_USEC = 10000;


CNetServerReactor::CNetServerReactor()
: m_iCurrentUDPSvrSockIndex(1)
, m_iCurrentUDPPeerAddrIndex(1)
, m_pUDPNetHandler(NULL)
, m_iCurrentTCPSvrSockIndex(1)
, m_iCurrentTimerIndex(1)
, m_bEventStop(true)
, m_iNumOfFds(0)
, m_bNeedTimer(true)
, m_bNeedUDP(true)
, m_bNeedTCPServer(true)
, m_bNeedTCPServerAccept(true)
, m_bNeedTCPClient(true)
, m_bNeedUserAction(true)
{
    m_stTimeVal.tv_sec = DEFAULT_SELECT_TIMEOUT_SEC;
    m_stTimeVal.tv_usec = DEFAULT_SELECT_TIMEOUT_USEC;

    m_l_iFromAddrLen = sizeof(struct sockaddr_in);

    FD_ZERO(&m_setReadFdsHolder);
    FD_ZERO(&m_setWriteFdsHolder);
}

CNetServerReactor::~CNetServerReactor()
{
    // Close all udp server sockets
    for(m_l_udp_svr_it = m_oUDPSvrSockInfoMap.begin()
        ; m_l_udp_svr_it != m_oUDPSvrSockInfoMap.end()
        ; m_l_udp_svr_it++)
    {
        close((*m_l_udp_svr_it).second.iSockfd);
    } 

    // Clean all tcp server & cloned TCPNetHandler
    ITCPNetHandler* l_pHandler = NULL;
    for(m_l_conn_it = m_oTCPSvrConnHandleMap.begin()
        ; m_l_conn_it != m_oTCPSvrConnHandleMap.end()
        ; m_l_conn_it++)
    {
        m_l_iSockfd = (*m_l_conn_it).first;
        close(m_l_iSockfd);

        // Callback
        l_pHandler = 
            m_oTCPSvrSockInfoMap[(*m_l_conn_it).second].oCloneTCPNetHandlerMap[m_l_iSockfd];

        //// Destroy cloned handler
        if(l_pHandler)
        {
            delete l_pHandler;
            m_oTCPSvrSockInfoMap[(*m_l_conn_it).second].oCloneTCPNetHandlerMap[m_l_iSockfd] = NULL;
        }
    }

    for(m_l_tcp_svr_it = m_oTCPSvrSockInfoMap.begin()
        ; m_l_tcp_svr_it != m_oTCPSvrSockInfoMap.end()
        ; m_l_tcp_svr_it++)
    {
        close((*m_l_tcp_svr_it).second.iSockfd);
    }
}

int CNetServerReactor::Initialize(
                                  bool bNeedTimer,
                                  bool bNeedUDP,
                                  bool bNeedTCPServer,
                                  bool bNeedTCPClient,
                                  bool bNeedUserAction)
{
    m_bNeedTimer = bNeedTimer;
    m_bNeedUDP = bNeedUDP;
    m_bNeedTCPServer = bNeedTCPServer;
    m_bNeedTCPClient = bNeedTCPClient;
    m_bNeedUserAction = bNeedUserAction;

    if(m_bNeedTCPServer)
        m_bNeedTCPServerAccept = true;
    else
        m_bNeedTCPServerAccept = false;

    return 0; 
}

///////////// UDP ////////////////
int CNetServerReactor::RegisterUDPServer(
    int& iServerHandle, // [OUT]
    unsigned short ushPort, 
    const char* pszHost,
    bool bNoDelay,
	bool bFDWatch)
{
    if(!m_bNeedUDP)
    {
        m_sLastErrMsg = "[CNetServerReactor::RegisterUDPServer] m_bNeedUDP = false";
        return -1;
    }

    CUDPServerSockInfo oSvrSock;

    struct hostent* l_pHostInfo = NULL;
    struct in_addr* l_pAddp = NULL;

    if(pszHost) 
    {
        l_pHostInfo = ::gethostbyname(pszHost);
        if(l_pHostInfo == NULL)
        {
            m_sLastErrMsg = "[CNetServerReactor::RegisterUDPServer] gethostbyname failed. pszHost = ";
            m_sLastErrMsg += pszHost;
            return -1;
        }
    }

    oSvrSock.iSockfd = ::socket(AF_INET, SOCK_DGRAM, 0);
    if(oSvrSock.iSockfd < 0 )
    {
        m_sLastErrMsg = "[CNetServerReactor::RegisterUDPServer] create socket failed";
        return -1;
    }

    if (l_pHostInfo) 
    {
        l_pAddp = reinterpret_cast<struct in_addr*>(*(l_pHostInfo->h_addr_list));
        oSvrSock.stINETAddr.sin_addr = *l_pAddp;
    } 
    else 
    {
        oSvrSock.stINETAddr.sin_addr.s_addr = INADDR_ANY;
    }

    oSvrSock.stINETAddr.sin_family = AF_INET;
    oSvrSock.stINETAddr.sin_port = htons(ushPort);

    const int on = 1;
    setsockopt(oSvrSock.iSockfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

    if ((::bind(
        oSvrSock.iSockfd, 
        (struct sockaddr *)&(oSvrSock.stINETAddr),
        sizeof(struct sockaddr))) == -1) 
    {
        close(oSvrSock.iSockfd);
        m_sLastErrMsg = "[CNetServerReactor::RegisterUDPServer] bind socket failed";
        return -1;
    }

    // Set NO DELAY
    if(bNoDelay)
    {
        int iCurrentFlag = 0;
        if((iCurrentFlag = fcntl(oSvrSock.iSockfd, F_GETFL, 0)) == -1)
        {
            close(oSvrSock.iSockfd);
            m_sLastErrMsg = "[CNetServerReactor::RegisterUDPServer] fcntl F_GETFL failed";
            return -1;
        }

        if (fcntl(oSvrSock.iSockfd, F_SETFL, iCurrentFlag | FNDELAY) == -1)
        {
            close(oSvrSock.iSockfd);
            m_sLastErrMsg = "[CNetServerReactor::RegisterServer] fcntl F_SETFL failed";
            return -1;
        }
    }

    m_oUDPSvrSockInfoMap[m_iCurrentUDPSvrSockIndex] = oSvrSock;
    iServerHandle = m_iCurrentUDPSvrSockIndex++;

    if(oSvrSock.iSockfd >= m_iNumOfFds)
        m_iNumOfFds = oSvrSock.iSockfd + 1;

	if(bFDWatch)
		FD_SET(oSvrSock.iSockfd, &m_setReadFdsHolder);

    return 0;
}

int CNetServerReactor::RegisterPeerAddr(
                                        int& iPeerHandle,   // [OUT]
                                        const char* pszHost,
                                        unsigned short ushPort)
{
    if(!m_bNeedUDP)
    {
        m_sLastErrMsg = "[CNetServerReactor::RegisterPeerAddr] m_bNeedUDP = false";
        return -1;
    }

    struct sockaddr_in oSockAddr;

    ::memset(&oSockAddr, 0, sizeof(struct sockaddr_in));

    oSockAddr.sin_family = AF_INET;
    oSockAddr.sin_addr.s_addr = inet_addr(pszHost);
    oSockAddr.sin_port = htons(ushPort);

    m_oUDPPeerAddrInfoMap[m_iCurrentUDPPeerAddrIndex] = oSockAddr;
    iPeerHandle = m_iCurrentUDPPeerAddrIndex++;

    return 0;
}

int CNetServerReactor::SendTo(
                              int iServerHandle,
                              const char* pSendBuf, 
                              uint32_t nBufLen, 
                              int iPeerHandle,
                              int iFlag)
{
    if(!m_bNeedUDP)
    {
        m_sLastErrMsg = "[CNetServerReactor::SendTo] m_bNeedUDP = false";
        return -1;
    }

    // Check Server Handle
    std::map<int, UDPServerSockInfo_T>::const_iterator ItSvr 
        = m_oUDPSvrSockInfoMap.find(iServerHandle);

    if(ItSvr == m_oUDPSvrSockInfoMap.end())
    {
        m_sLastErrMsg = "[CNetServerReactor::SendTo] unregisted sever handle. iServerHandle = ";
        std::ostringstream oss;
        oss << iServerHandle;
        m_sLastErrMsg += oss.str();

        return -1;
    }

    // Check Peer Handle
    std::map<int, sockaddr_in>::const_iterator ItPeer
        = m_oUDPPeerAddrInfoMap.find(iPeerHandle);

    if(ItPeer == m_oUDPPeerAddrInfoMap.end())
    {
        m_sLastErrMsg = "[CNetServerReactor::SendTo] unregisted peer handle. iPeerHandle = ";
        std::ostringstream oss;
        oss << iPeerHandle;
        m_sLastErrMsg += oss.str();

        return -1;
    }

    // Send Buffer
    int iBytesSent = ::sendto(
        (*ItSvr).second.iSockfd,
        pSendBuf,
        nBufLen,
        iFlag,
        (struct sockaddr *)&((*ItPeer).second),
        sizeof(struct sockaddr_in));

    if(iBytesSent == -1 || static_cast<uint32_t>(iBytesSent) != nBufLen)
    {
        m_sLastErrMsg = "[CNetServerReactor::SendTo] sendto peer failed. errno = ";
        std::ostringstream oss1;
        oss1 << errno;
        m_sLastErrMsg += oss1.str();

        return -1;
    }

    return 0;
}

int CNetServerReactor::SendTo(
                              int iServerHandle,
                              const char* pSendBuf, 
                              uint32_t nBufLen, 
                              const char* pszHost,
                              unsigned short ushPort,
                              int iFlag)
{
    if(!m_bNeedUDP)
    {
        m_sLastErrMsg = "[CNetServerReactor::SendTo] m_bNeedUDP = false";
        return -1;
    }

    // Check Server Handle
    std::map<int, UDPServerSockInfo_T>::const_iterator ItSvr 
        = m_oUDPSvrSockInfoMap.find(iServerHandle);

    if(ItSvr == m_oUDPSvrSockInfoMap.end())
    {
        m_sLastErrMsg = "[CNetServerReactor::SendTo] unregisted sever handle. iServerHandle = ";
        std::ostringstream oss;
        oss << iServerHandle;
        m_sLastErrMsg += oss.str();

        return -1;
    }

    // Make Peer Addr
    struct sockaddr_in oSockAddr;
    ::memset(&oSockAddr, 0, sizeof(struct sockaddr_in));

    oSockAddr.sin_family = AF_INET;
    oSockAddr.sin_addr.s_addr = inet_addr(pszHost);
    oSockAddr.sin_port = htons(ushPort);

    // Send Buffer
    int iBytesSent = ::sendto(
        (*ItSvr).second.iSockfd,
        pSendBuf,
        nBufLen,
        iFlag,
        (struct sockaddr*)&oSockAddr,
        sizeof(struct sockaddr_in));

    if(iBytesSent == -1 || static_cast<uint32_t>(iBytesSent) != nBufLen)
    {
        m_sLastErrMsg = "[CNetServerReactor::SendTo] sendto peer failed. errno = ";
        std::ostringstream oss1;
        oss1 << errno;
        m_sLastErrMsg += oss1.str();
        m_sLastErrMsg += ", Host = ";
        m_sLastErrMsg += pszHost;
        m_sLastErrMsg += ", Port = ";
        std::ostringstream oss2;
        oss2 << ushPort;
        m_sLastErrMsg += oss2.str();

        return -1;
    }

    return 0;
}

/////////////////// TCP ///////////////////
int CNetServerReactor::RegisterTCPServer(
    int& iServerHandle, // [OUT]
    ITCPNetHandler* pHandler,
    unsigned short ushPort, 
    bool bParallel,
    int iRcvBufLen,
    const char* pszHost,
    bool bNoDelay,
	bool bFDWatch)
{
    if(!m_bNeedTCPServer)
    {
        m_sLastErrMsg = "[CNetServerReactor::RegisterTCPServer] m_bNeedTCPServer = false";
        return -1;
    }

    assert(pHandler != NULL);

    CTCPServerSockInfo oSvrSock;

    struct hostent* l_pHostInfo = NULL;
    struct in_addr* l_pAddp = NULL;

    if(pszHost) 
    {
        l_pHostInfo = ::gethostbyname(pszHost);
        if(l_pHostInfo == NULL)
        {
            m_sLastErrMsg = "[CNetServerReactor::RegisterTCPServer] gethostbyname failed. pszHost = ";
            m_sLastErrMsg += pszHost;
            return -1;
        }
    }

    oSvrSock.iSockfd = ::socket(AF_INET, SOCK_STREAM, 0);
    if(oSvrSock.iSockfd < 0 )
    {
        m_sLastErrMsg = "[CNetServerReactor::RegisterTCPServer] create socket failed";
        return -1;
    }

    if (l_pHostInfo) 
    {
        l_pAddp = reinterpret_cast<struct in_addr*>(*(l_pHostInfo->h_addr_list));
        oSvrSock.stINETAddr.sin_addr = *l_pAddp;
    } 
    else 
    {
        oSvrSock.stINETAddr.sin_addr.s_addr = INADDR_ANY;
    }

    oSvrSock.stINETAddr.sin_family = AF_INET;
    oSvrSock.stINETAddr.sin_port = htons(ushPort);

    oSvrSock.pHandler = pHandler;   // Set callback cloneable handler
    oSvrSock.bParallel = bParallel;

    const int on = 1;
    setsockopt(oSvrSock.iSockfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

    if ((::bind(
        oSvrSock.iSockfd, 
        (struct sockaddr *)&(oSvrSock.stINETAddr),
        sizeof(struct sockaddr))) == -1) 
    {
        close(oSvrSock.iSockfd);
        m_sLastErrMsg = "[CNetServerReactor::RegisterTCPServer] bind socket failed";
        return -1;
    }

    // Set NO DELAY
    if(bNoDelay)
    {
        linger m_sLinger;
        m_sLinger.l_onoff = 1;  // 在closesocket()调用,但是还有数据没发送完毕的时候容许逗留
        m_sLinger.l_linger = 0; // 容许逗留的时间为0秒
        setsockopt(oSvrSock.iSockfd, SOL_SOCKET, SO_LINGER, (const char*)&m_sLinger, sizeof(linger));


        int iCurrentFlag = 0;
        if((iCurrentFlag = fcntl(oSvrSock.iSockfd, F_GETFL, 0)) == -1)
        {
            close(oSvrSock.iSockfd);
            m_sLastErrMsg = "[CNetServerReactor::RegisterTCPServer] fcntl F_GETFL failed";
            return -1;
        }

        if (fcntl(oSvrSock.iSockfd, F_SETFL, iCurrentFlag | FNDELAY) == -1)
        {
            close(oSvrSock.iSockfd);
            m_sLastErrMsg = "[CNetServerReactor::RegisterTCPServer] fcntl F_SETFL failed";
            return -1;
        }
    }

    int iBacklog = 32;   // Max pending connection
    if(listen(oSvrSock.iSockfd, iBacklog) < 0)
    {
        close(oSvrSock.iSockfd);
        m_sLastErrMsg = "[CNetServerReactor::RegisterTCPServer] listen failed";
        return -1;
    }

    oSvrSock.iRcvBufLen = iRcvBufLen;

	m_oTCPSvrSockInfoMap[m_iCurrentTCPSvrSockIndex] = oSvrSock;
	iServerHandle = m_iCurrentTCPSvrSockIndex++;

	if(oSvrSock.iSockfd >= m_iNumOfFds)
		m_iNumOfFds = oSvrSock.iSockfd + 1;

	if(bFDWatch)
		FD_SET(oSvrSock.iSockfd, &m_setReadFdsHolder);

    return 0;
}

int CNetServerReactor::ConnectToServer(
                                       ITCPNetHandler* pHandler,
                                       unsigned short ushServerPort, 
                                       const char* pszServerHost,
				                       uint32_t dwConnTimeout
                                       )
{
    dwConnTimeout = (dwConnTimeout == 0 ? 10 : dwConnTimeout);
    
    if(!m_bNeedTCPClient)
    {
        m_sLastErrMsg = "[CNetServerReactor::ConnectToServer] m_bNeedTCPClient = false";
        return -1;
    }

    assert(pHandler != NULL);

    CTCPServerSockInfo oSvrSock;

    struct hostent* l_pHostInfo = NULL;
    struct in_addr* l_pAddp = NULL;

    if(pszServerHost) 
    {
        l_pHostInfo = ::gethostbyname(pszServerHost);
        if(l_pHostInfo == NULL)
        {
            m_sLastErrMsg = "[CNetServerReactor::ConnectToServer] gethostbyname failed. pszServerHost = ";
            m_sLastErrMsg += pszServerHost;
            return -1;
        }
    }

    oSvrSock.iSockfd = ::socket(AF_INET, SOCK_STREAM, 0);
    if(oSvrSock.iSockfd < 0 )
    {
        m_sLastErrMsg = "[CNetServerReactor::ConnectToServer] create socket failed";
        return -1;
    }

    const int on = 1;
    setsockopt(oSvrSock.iSockfd, SOL_SOCKET, SO_KEEPALIVE, &on, sizeof(on));

    l_pAddp = reinterpret_cast<struct in_addr*>(*(l_pHostInfo->h_addr_list));
    oSvrSock.stINETAddr.sin_addr = *l_pAddp;

    oSvrSock.stINETAddr.sin_family = AF_INET;
    oSvrSock.stINETAddr.sin_port = htons(ushServerPort);

    oSvrSock.pHandler = pHandler;   // Set callback handler

    if(dwConnTimeout) //如果设置了超时时间
    {
    	int flags = 0;
    	flags = fcntl(oSvrSock.iSockfd, F_GETFL, 0);
    	fcntl(oSvrSock.iSockfd, F_SETFL, flags | O_NONBLOCK);
    }

    int iret = connect(
	    oSvrSock.iSockfd, 
	    (struct sockaddr *)&oSvrSock.stINETAddr, 
	    sizeof(oSvrSock.stINETAddr));
    if(iret < 0)
    {
	if(dwConnTimeout == 0 || errno != EINPROGRESS)
	{
	    m_sLastErrMsg = "[CNetServerReactor::ConnectToServer] connect to server ";
	    m_sLastErrMsg += pszServerHost;
	    m_sLastErrMsg += " failed";

	    close(oSvrSock.iSockfd);
	    return -1;
	}

	struct timeval tv;
	tv.tv_sec = dwConnTimeout / 1000;
	tv.tv_usec = (dwConnTimeout - tv.tv_sec * 1000) * 1000;

	fd_set stWtFds;
	FD_ZERO(&stWtFds);
	FD_SET(oSvrSock.iSockfd, &stWtFds);

	int iFdsNum = oSvrSock.iSockfd + 1;
	if(select(iFdsNum, NULL, &stWtFds, NULL, &tv) < 0)
	{
        std::stringstream oss;
        oss << "[CNetServerReactor::ConnectToServer] select ";
	    oss << pszServerHost << ":" << ushServerPort << ",";
	    oss << " timeout:" << dwConnTimeout;
        oss << ", errno:" << errno << ", strerror:" << strerror(errno);
        m_sLastErrMsg = oss.str();
        
	    close(oSvrSock.iSockfd);
	    return -1;
	}

	if(!FD_ISSET(oSvrSock.iSockfd, &stWtFds))
	{
	    std::stringstream oss;
        oss << "[CNetServerReactor::ConnectToServer] FD_ISSET ";
	    oss << pszServerHost << ":" << ushServerPort << ",";
	    oss << " timeout:" << dwConnTimeout;
        oss << ", errno:" << errno << ", strerror:" << strerror(errno);
        m_sLastErrMsg = oss.str();

	    close(oSvrSock.iSockfd);
	    return -1;
	}	

	int error = 0;
	socklen_t len = sizeof(error);
	if(getsockopt(oSvrSock.iSockfd, SOL_SOCKET, SO_ERROR, &error, &len) < 0 || error)
	{
	    std::stringstream oss;
        oss << "[CNetServerReactor::ConnectToServer] connect to server3 ";
	    oss << pszServerHost << ":" << ushServerPort << ",";
	    oss << " timeout:" << dwConnTimeout;
        oss << ", errno:" << errno << ", strerror:" << strerror(errno);
        m_sLastErrMsg = oss.str();

	    close(oSvrSock.iSockfd);

	    return -1;
	}
    }

    int iCurrentFlag = 0;
    if((iCurrentFlag = fcntl(oSvrSock.iSockfd, F_GETFL, 0)) == -1)
    {
        close(oSvrSock.iSockfd);
        m_sLastErrMsg = "[CNetServerReactor::ConnectToServer] fcntl F_GETFL failed";
        return -1;
    }

    if (fcntl(oSvrSock.iSockfd, F_SETFL, iCurrentFlag | FNDELAY) == -1)
    {
        close(oSvrSock.iSockfd);
        m_sLastErrMsg = "[CNetServerReactor::ConnectToServer] fcntl F_SETFL failed";
        return -1;
    }

    // Add this oSvrSock.iSockfd to event loop
    if(oSvrSock.iSockfd >= m_iNumOfFds)
        m_iNumOfFds = oSvrSock.iSockfd + 1;

    FD_SET(oSvrSock.iSockfd, &m_setReadFdsHolder);

    m_oTCPClientConnHandleMap[oSvrSock.iSockfd] = oSvrSock;

    // Callback
    pHandler->OnConnect(
        oSvrSock.iSockfd,  // ConnHandler
        oSvrSock.stINETAddr.sin_addr.s_addr,
        ushServerPort,
        0);

    return 0;
}

int CNetServerReactor::CheckTCPComingConnection_i()
{
    int l_iSockFd = 0;
    unsigned int l_uiHost = 0;
    unsigned short l_ushPort = 0;
    ITCPNetHandler *l_pHandler = NULL;

    for(m_l_tcp_svr_it = m_oTCPSvrSockInfoMap.begin()
        ; m_l_tcp_svr_it != m_oTCPSvrSockInfoMap.end()
        ; m_l_tcp_svr_it++)
    {
        m_l_iSockfd = (*m_l_tcp_svr_it).second.iSockfd;
        if(FD_ISSET(m_l_iSockfd, &m_setReadFds))
        {
            TCPServerSockInfo_T& rTcpSvrSockInfo = (*m_l_tcp_svr_it).second;
            l_iSockFd = accept(
                rTcpSvrSockInfo.iSockfd, 
                (struct sockaddr*)&m_l_stFromAddr, 
                &m_l_iFromAddrLen);

            if(l_iSockFd < 0)
            {
                if(errno != EAGAIN && errno != EWOULDBLOCK)
                {
                    //ERROR_TRACE("TCP Server Accept failed");
                }

                continue;
            }

            if(rTcpSvrSockInfo.bParallel)
            {
                int iPid = 0;
                if ((iPid = fork()) < 0) 
                {
                    return -1;
                }

                if(iPid > 0)    // main process
                {
                    close(l_iSockFd);
                    continue;
                }

                if(iPid == 0)   // new child process
                {
                    // Do not accept new connection further,
                    //// Only let main process do accept
                    m_bNeedTCPServerAccept = false;   
                }
            }

            l_uiHost = m_l_stFromAddr.sin_addr.s_addr;
            l_ushPort = ntohs(m_l_stFromAddr.sin_port);

            // Add this SockFd to event loop
            if(l_iSockFd >= m_iNumOfFds)
                m_iNumOfFds = l_iSockFd + 1;

            FD_SET(l_iSockFd, &m_setReadFdsHolder);

            m_oTCPSvrConnHandleMap[l_iSockFd] = (*m_l_tcp_svr_it).first;

            // Callback
            l_pHandler = rTcpSvrSockInfo.pHandler->Clone();
            rTcpSvrSockInfo.oCloneTCPNetHandlerMap[l_iSockFd] = l_pHandler;

            // Set Large RecvBuf for this socket
            //std::cout << "rTcpSvrSockInfo.ulRcvBufLen[" << rTcpSvrSockInfo.iRcvBufLen << "]" << std::endl;
            setsockopt(
                l_iSockFd, 
                SOL_SOCKET, 
                SO_RCVBUF, 
                reinterpret_cast<const void*>(&rTcpSvrSockInfo.iRcvBufLen),
                sizeof(rTcpSvrSockInfo.iRcvBufLen)
                );

            // Set NO DELAY
            linger m_sLinger;
            m_sLinger.l_onoff = 1;  // 在closesocket()调用,但是还有数据没发送完毕的时候容许逗留
            m_sLinger.l_linger = 0; // 容许逗留的时间为0秒
            setsockopt(l_iSockFd, SOL_SOCKET, SO_LINGER, (const char*)&m_sLinger, sizeof(linger));

			const int on = 1;
			setsockopt(l_iSockFd, SOL_SOCKET, SO_KEEPALIVE, &on, sizeof(on));

            int iCurrentFlag = fcntl(l_iSockFd, F_GETFL, 0);
            fcntl(l_iSockFd, F_SETFL, iCurrentFlag | FNDELAY);

            l_pHandler->OnConnect(
                l_iSockFd,  // ConnHandler
                l_uiHost,
                l_ushPort,
                0 );
        }
    }

    return 0;
}

int CNetServerReactor::Send(
                               int iConnHandle,
                               char* pSendBuf, 
                               uint32_t nBufLen, 
                               int iFlag)
{
    if(!m_bNeedTCPServer && !m_bNeedTCPClient)
    {
        m_sLastErrMsg = "[CNetServerReactor::Send] !m_bNeedTCPServer && !m_bNeedTCPClient";
        return 0;
    }    

    // Send Buffer
    int iBytesSent = ::send(
        iConnHandle,
        pSendBuf,
        nBufLen,
        iFlag);

    if(iBytesSent == -1)
    {
		if(errno != EINTR && errno != EAGAIN && errno != EWOULDBLOCK)
		{
			m_sLastErrMsg = "[CNetServerReactor::Send] sendto peer failed. errno = ";
			std::ostringstream oss1;
			oss1 << errno;
			m_sLastErrMsg += oss1.str();
			return -1;
		}

        return 0;
    }

    return iBytesSent;
}

int CNetServerReactor::Close(int iConnHandle, bool bAsServer)
{
    if(!m_bNeedTCPServer && !m_bNeedTCPClient)
    {
        m_sLastErrMsg = "[CNetServerReactor::Close] !m_bNeedTCPServer && !m_bNeedTCPClient";
        return -1;
    }  

    ::close(iConnHandle);

    FD_CLR(iConnHandle, &m_setReadFdsHolder);
    FD_CLR(iConnHandle, &m_setWriteFdsHolder);

    if(bAsServer)
    {
        // Parallel server, one connection -- one Process
        // Shutdown this Process when connection down.
        if(m_oTCPSvrSockInfoMap[m_oTCPSvrConnHandleMap[iConnHandle]].bParallel)
        {
            m_bEventStop = true;
        }
    }

    return 0;
}

void CNetServerReactor::NeedOnSendCheck(int iConnHandle)
{
    FD_SET(iConnHandle, &m_setWriteFdsHolder);
}
/////////////////// Timer ///////////////////
int CNetServerReactor::RegisterTimer(
                                     int& iTimerID,      // [OUT]
                                     ITimerHandler* pHandler,
                                     const CTimeValue& tvInterval,
                                     unsigned int dwCount)
{
    if(!m_bNeedTimer)
    {
        m_sLastErrMsg = "[CNetServerReactor::RegisterTimer] m_bNeedTimer = false";
        return -1;
    } 

    int iRetCode = m_oTimerQueue.ScheduleTimer(
        pHandler, 
        ++m_iCurrentTimerIndex, 
        tvInterval, 
        dwCount);

    if(iRetCode != 0)
    {
        m_sLastErrMsg = "[CNetServerReactor::RegisterTimer] ScheduleTimer failed";
        return -1;
    }

    iTimerID = m_iCurrentTimerIndex;

    return 0;
}

int CNetServerReactor::UnregisterTimer(int iTimerID)
{
    if(!m_bNeedTimer)
    {
        m_sLastErrMsg = "[CNetServerReactor::UnregisterTimer] m_bNeedTimer = false";
        return -1;
    } 

    int iRetCode = m_oTimerQueue.CancelTimer(iTimerID);
    if(iRetCode != 0)
    {
        m_sLastErrMsg = "[CNetServerReactor::UnregisterTimer] CancelTimer failed, iTimerID = ";
        return -1;
    }

    return 0;
}

////////////// Register Event Handler ///////////////
void CNetServerReactor::RegisterUDPNetHandler(
    IUDPNetHandler* pHandler)
{
    if(!m_bNeedUDP)
    {
        m_sLastErrMsg = "[CNetServerReactor::RegisterUDPNetHandler] m_bNeedUDP = false";
        return;
    } 

    m_pUDPNetHandler = pHandler;
}

void CNetServerReactor::RegisterUserEventHandler(
    IUserEventHandler* pUserEventHandler
    )
{
    if(!m_bNeedUserAction)
    {
        m_sLastErrMsg = "[CNetServerReactor::RegisterUserEventHandler] m_bNeedUserAction = false";
        return;
    } 

    m_pUserEventHandler = pUserEventHandler;
}

/////////////////// Run Event Loop///////////////////
void CNetServerReactor::RunEventLoop()
{
    m_bEventStop = false;

    while(!m_bEventStop)
    {
        if(this->CheckEvents() < 0)		
            continue;

        this->ProcessSocketEvent();
    }
}

void CNetServerReactor::StopEventLoop()
{
    m_bEventStop = true;
}

int CNetServerReactor::CheckEvents()
{
    if(m_bNeedUserAction && m_pUserEventHandler)
    {
        if(m_pUserEventHandler->CheckEvent() == 0)
            m_pUserEventHandler->OnEventFire();
    }

    // Check Timer first
    if(m_bNeedTimer)
    {
        m_l_tvRemain = CTimeValue::s_tvMax;
        m_oTimerQueue.CheckExpire(&m_l_tvRemain);
    }

    // Check SocketStatus
    FD_ZERO(&m_setReadFds);
    FD_ZERO(&m_setWriteFds);
    m_setReadFds = m_setReadFdsHolder;
    m_setWriteFds = m_setWriteFdsHolder;

    // Set default timeout for select
    m_stTimeVal.tv_sec = DEFAULT_SELECT_TIMEOUT_SEC;
    m_stTimeVal.tv_usec = DEFAULT_SELECT_TIMEOUT_USEC;

    if (select(
        m_iNumOfFds, 
        &m_setReadFds, 
        &m_setWriteFds
        , NULL, &m_stTimeVal) > 0)
    {
        return 0;
    }

    return -1;
}

int CNetServerReactor::ProcessSocketEvent()
{
    unsigned int l_uiHost = 0;
    unsigned short l_ushPort = 0;

    if(m_bNeedTCPServerAccept)
    {
        CheckTCPComingConnection_i();
    }
    else if(m_bNeedUDP)
    {
        // Process UDP Event
        for(m_l_udp_svr_it = m_oUDPSvrSockInfoMap.begin()
            ; m_l_udp_svr_it != m_oUDPSvrSockInfoMap.end()
            ; m_l_udp_svr_it++)
        {
            m_l_iSockfd = (*m_l_udp_svr_it).second.iSockfd;

            if(FD_ISSET(m_l_iSockfd, &m_setReadFds))
            {
                m_iRecvBufLen = recvfrom(
                    m_l_iSockfd, 
                    m_achRecvBuf, 
                    MAX_RECV_BUF_LEN, 
                    0, 
                    (struct sockaddr*)&m_l_stFromAddr, 
                    &m_l_iFromAddrLen);

                if(m_iRecvBufLen <= 0)
                    continue;

                l_uiHost = ntohl(m_l_stFromAddr.sin_addr.s_addr);
                l_ushPort = ntohs(m_l_stFromAddr.sin_port);

                // Callback
                if(m_pUDPNetHandler)
                {
                    m_pUDPNetHandler->OnRecvFrom(
                        (*m_l_udp_svr_it).first, 
                        m_achRecvBuf,
                        m_iRecvBufLen,
                        l_uiHost,
                        l_ushPort,
                        0);
                }
            }
        }
    }

    if(m_bNeedTCPServer)
    {
        // Process TCP Server Event
        ITCPNetHandler* l_pHandler = NULL;
        for(m_l_conn_it = m_oTCPSvrConnHandleMap.begin()
            ; m_l_conn_it != m_oTCPSvrConnHandleMap.end()
            ; m_l_conn_it++)
        {
            m_l_iSockfd = (*m_l_conn_it).first;
            if(FD_ISSET(m_l_iSockfd, &m_setReadFds))
            {
                m_iRecvBufLen = recv(
                    m_l_iSockfd, 
                    m_achRecvBuf, 
                    MAX_RECV_BUF_LEN, 
                    MSG_NOSIGNAL);

                if(m_iRecvBufLen <= 0)
                {
                    if(m_iRecvBufLen < 0 && (errno == EAGAIN || errno == EINTR))
                    {
                        continue;
                    }

                    // Callback
                    l_pHandler = m_oTCPSvrSockInfoMap[(*m_l_conn_it).second].oCloneTCPNetHandlerMap[m_l_iSockfd];
					if(l_pHandler) /// Reactor as both Client & Server, Sockfd may reused
					{
						l_pHandler->OnClose(m_l_iSockfd, 0, 0);
						this->Close(m_l_iSockfd, true);

						//// Destroy cloned handler
						delete l_pHandler;
						m_oTCPSvrSockInfoMap[(*m_l_conn_it).second].oCloneTCPNetHandlerMap[m_l_iSockfd] = NULL;
					}

                    continue;
                }

                // Callback
                l_pHandler = m_oTCPSvrSockInfoMap[(*m_l_conn_it).second].oCloneTCPNetHandlerMap[m_l_iSockfd];
                if(l_pHandler)
				{
					l_pHandler->OnRecv(
							m_l_iSockfd,
							m_achRecvBuf,
							m_iRecvBufLen,
							0);
				}
            }
            else if(FD_ISSET(m_l_iSockfd, &m_setWriteFds))
            {
                // Callback
                l_pHandler =
                    m_oTCPSvrSockInfoMap[(*m_l_conn_it).second].oCloneTCPNetHandlerMap[m_l_iSockfd];
                FD_CLR(m_l_iSockfd, &m_setWriteFdsHolder);
                l_pHandler->OnSend(m_l_iSockfd);
            }
        }
    }

    if(m_bNeedTCPClient)
    {
        // Process TCP Client Event
        ITCPNetHandler* l_pHandler = NULL;
        for(m_l_tcp_cli_it = m_oTCPClientConnHandleMap.begin()
            ; m_l_tcp_cli_it != m_oTCPClientConnHandleMap.end()
            ; m_l_tcp_cli_it++)
        {
            m_l_iSockfd = (*m_l_tcp_cli_it).first;
            if(FD_ISSET(m_l_iSockfd, &m_setReadFds))
            {
                m_iRecvBufLen = recv(
                    m_l_iSockfd, 
                    m_achRecvBuf, 
                    MAX_RECV_BUF_LEN, 
                    0);

                if(m_iRecvBufLen <= 0)
                {
                    if(m_iRecvBufLen < 0 && (errno == EAGAIN || errno == EINTR))
                    {
                        continue;
                    }

					l_pHandler = (*m_l_tcp_cli_it).second.pHandler;
                    if(l_pHandler->IsConnected())
					{
						// Callback
						l_pHandler->OnClose(m_l_iSockfd, 0, 0);
						this->Close(m_l_iSockfd, false);
					}

                    continue;
                }

				l_pHandler = (*m_l_tcp_cli_it).second.pHandler;
				if(l_pHandler->IsConnected())
				{
					// Callback
					l_pHandler->OnRecv(
							m_l_iSockfd,
							m_achRecvBuf,
							m_iRecvBufLen,
							0);
				}
            }
            else if(FD_ISSET(m_l_iSockfd, &m_setWriteFds))
            {
                // Callback
                l_pHandler = 
                    (*m_l_tcp_cli_it).second.pHandler;
                FD_CLR(m_l_iSockfd, &m_setWriteFdsHolder);
                l_pHandler->OnSend(m_l_iSockfd);
            }
        }
    }

    return 0;
}

const char* CNetServerReactor::GetLastErrMsg() const
{
    return m_sLastErrMsg.c_str();
}

int CNetServerReactor::RegisterUSockTCPServer(
    int& iServerHandle, // [OUT]
    ITCPNetHandler* pHandler,
    const char* pszSockPath,
    bool bParallel, // Multi-Process server
    int iRcvBufLen,
    bool bNoDelay,
	mode_t mask,
	bool bFDWatch)
{
    if(!m_bNeedTCPServer)
    {
        m_sLastErrMsg = "[CNetServerReactor::RegisterUSockServer] m_bNeedTCPServer = false";
        return -1;
    }

    assert(pszSockPath != NULL);
    assert(pHandler != NULL);

    int iRetCode = 0;
    CTCPServerSockInfo oSvrSock;

    oSvrSock.iSockfd = ::socket(AF_UNIX, SOCK_STREAM, 0);
    if(oSvrSock.iSockfd < 0 )
    {
        m_sLastErrMsg = "[CNetServerReactor::RegisterUSockServer] create socket failed";
        return -1;
    }

    oSvrSock.pHandler = pHandler;   // Set callback cloneable handler
    oSvrSock.bParallel = bParallel;

    mode_t old_mod = umask(mask); // Set umask = 002

    memset(&oSvrSock.stUNIXAddr, 0, sizeof(oSvrSock.stUNIXAddr));
    oSvrSock.stUNIXAddr.sun_family = AF_UNIX;
    StrMov(oSvrSock.stUNIXAddr.sun_path, pszSockPath); // "/tmp/pipe_channel.sock"
    unlink(pszSockPath); // unlink it if exist

    const int on = 1;
    setsockopt(oSvrSock.iSockfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

    if ((::bind(
        oSvrSock.iSockfd, 
        (struct sockaddr *)&(oSvrSock.stUNIXAddr),
        sizeof(struct sockaddr))) == -1) 
    {
        close(oSvrSock.iSockfd);
        m_sLastErrMsg = "[CNetServerReactor::RegisterUSockServer] bind socket failed";
        return -1;
    }

    umask(old_mod); // Set back umask

    // Set NO DELAY
    if(bNoDelay)
    {
        linger m_sLinger;
        m_sLinger.l_onoff = 1;  // 在closesocket()调用,但是还有数据没发送完毕的时候容许逗留
        m_sLinger.l_linger = 0; // 容许逗留的时间为0秒
        iRetCode = setsockopt(
            oSvrSock.iSockfd, 
            SOL_SOCKET, 
            SO_LINGER, 
            reinterpret_cast<const char*>(&m_sLinger), 
            sizeof(linger));
        if(iRetCode < 0) 
        {
            close(oSvrSock.iSockfd);
            m_sLastErrMsg = "[CNetServerReactor::RegisterUSockServer] Set Socket Linger failed";
            return -1;
        }

        int iCurrentFlag = 0;
        if((iCurrentFlag = fcntl(oSvrSock.iSockfd, F_GETFL, 0)) == -1)
        {
            close(oSvrSock.iSockfd);
            m_sLastErrMsg = "[CNetServerReactor::RegisterUSockServer] fcntl F_GETFL failed";
            return -1;
        }

        if (fcntl(oSvrSock.iSockfd, F_SETFL, iCurrentFlag | FNDELAY) == -1)
        {
            close(oSvrSock.iSockfd);
            m_sLastErrMsg = "[CNetServerReactor::RegisterUSockServer] fcntl F_SETFL failed";
            return -1;
        }
    }

    int iBacklog = 32;   // Max pending connection
    if(listen(oSvrSock.iSockfd, iBacklog) < 0)
    {
        close(oSvrSock.iSockfd);
        m_sLastErrMsg = "[CNetServerReactor::RegisterUSockServer] listen failed";
        return -1;
    }

    oSvrSock.iRcvBufLen = iRcvBufLen;

	m_oTCPSvrSockInfoMap[m_iCurrentTCPSvrSockIndex] = oSvrSock;
	iServerHandle = m_iCurrentTCPSvrSockIndex++;

	if(oSvrSock.iSockfd >= m_iNumOfFds)
		m_iNumOfFds = oSvrSock.iSockfd + 1;

	if(bFDWatch)
		FD_SET(oSvrSock.iSockfd, &m_setReadFdsHolder);

    return 0;
}

int CNetServerReactor::ConnectToUSockTCPServer(
    ITCPNetHandler* pHandler,
    const char* pszSockPath,
    uint32_t dwConnTimeout
    )
{
    if(!m_bNeedTCPClient)
    {
        m_sLastErrMsg = "[CNetServerReactor::ConnectToUSockServer] m_bNeedTCPClient = false";
        return -1;
    }

    assert(pszSockPath != NULL);
    assert(pHandler != NULL);

    CTCPServerSockInfo oSvrSock;

    oSvrSock.iSockfd = ::socket(AF_UNIX, SOCK_STREAM, 0);
    if(oSvrSock.iSockfd < 0 )
    {
        m_sLastErrMsg = "[CNetServerReactor::ConnectToUSockServer] create socket failed";
        return -1;
    }

    memset(&oSvrSock.stUNIXAddr, 0, sizeof(oSvrSock.stUNIXAddr));
    oSvrSock.stUNIXAddr.sun_family = AF_UNIX;
    StrMov(oSvrSock.stUNIXAddr.sun_path, pszSockPath); // "/tmp/pipe_channel.sock"

    oSvrSock.pHandler = pHandler;   // Set callback handler

    if(dwConnTimeout) //如果设置了超时时间
    {
	int flags = 1;
	flags = fcntl(oSvrSock.iSockfd, F_GETFL, 0);
	fcntl(oSvrSock.iSockfd, F_SETFL, flags | O_NONBLOCK);
    }

    int iret = connect(
	    oSvrSock.iSockfd, 
	    (struct sockaddr *)&oSvrSock.stUNIXAddr, 
	    sizeof(oSvrSock.stUNIXAddr));
    if(iret < 0)
    {
	if(dwConnTimeout == 0 || errno != EINPROGRESS)
	{
	    m_sLastErrMsg = "[CNetServerReactor::ConnectToUSockServer] connect to usock server ";
	    m_sLastErrMsg += pszSockPath;
	    m_sLastErrMsg += " failed";

	    close(oSvrSock.iSockfd);
	    return -1;
	}

	struct timeval tv;
	tv.tv_sec = dwConnTimeout / 1000;
	tv.tv_usec = (dwConnTimeout - tv.tv_sec * 1000) * 1000;

	fd_set stWtFds;
	FD_ZERO(&stWtFds);
	FD_SET(oSvrSock.iSockfd, &stWtFds);

	int iFdsNum = oSvrSock.iSockfd + 1;
	if(select(iFdsNum, NULL, &stWtFds, NULL, &tv) < 0)
	{
	    m_sLastErrMsg = "[CNetServerReactor::ConnectToUSockServer] connect to usock server ";
	    m_sLastErrMsg += pszSockPath;
	    m_sLastErrMsg += " failed";

	    close(oSvrSock.iSockfd);
	    return -1;
	}

	if(!FD_ISSET(oSvrSock.iSockfd, &stWtFds))
	{
	    m_sLastErrMsg = "[CNetServerReactor::ConnectToServer] connect to server ";
	    m_sLastErrMsg += pszSockPath;
	    m_sLastErrMsg += " timeout";

	    close(oSvrSock.iSockfd);
	    return -1;
	}	

	int error = 0;
	socklen_t len = sizeof(error);
	if(getsockopt(oSvrSock.iSockfd, SOL_SOCKET, SO_ERROR, &error, &len) < 0 || error)
	{
	    m_sLastErrMsg = "[CNetServerReactor::ConnectToUSockServer] connect to usock server ";
	    m_sLastErrMsg += pszSockPath;
	    m_sLastErrMsg += " timeout";

	    close(oSvrSock.iSockfd);
	    return -1;
	}
    }

    int iCurrentFlag = 0;
    if((iCurrentFlag = fcntl(oSvrSock.iSockfd, F_GETFL, 0)) == -1)
    {
        close(oSvrSock.iSockfd);
        m_sLastErrMsg = "[CNetServerReactor::ConnectToServer] fcntl F_GETFL failed";
        return -1;
    }

    if (fcntl(oSvrSock.iSockfd, F_SETFL, iCurrentFlag | FNDELAY) == -1)
    {
        close(oSvrSock.iSockfd);
        m_sLastErrMsg = "[CNetServerReactor::ConnectToServer] fcntl F_SETFL failed";
        return -1;
    }

    // Add this oSvrSock.iSockfd to event loop
    if(oSvrSock.iSockfd >= m_iNumOfFds)
        m_iNumOfFds = oSvrSock.iSockfd + 1;

    FD_SET(oSvrSock.iSockfd, &m_setReadFdsHolder);

    m_oTCPClientConnHandleMap[oSvrSock.iSockfd] = oSvrSock;

    // Callback
    pHandler->OnConnect(
        oSvrSock.iSockfd,  // ConnHandler
        0,
        0,
        0);

    return 0;
}

int CNetServerReactor::RegisterUSockUDPServer(
    int& iServerHandle, // [OUT]
    const char* pszSockPath,
    bool bBind,
    bool bNoDelay,
	bool bFDWatch)
{
    assert(pszSockPath != NULL);

    CUDPServerSockInfo oSvrSock;

    oSvrSock.iSockfd = ::socket(PF_LOCAL, SOCK_DGRAM, 0);
    if(oSvrSock.iSockfd < 0 )
    {
        m_sLastErrMsg = "[CNetServerReactor::RegisterUSockUDPServer] create socket failed";
        return -1;
    }

    mode_t old_mod = umask(S_IRWXO); // Set umask = 002

    memset(&oSvrSock.stUNIXAddr, 0, sizeof(oSvrSock.stUNIXAddr));
    oSvrSock.stUNIXAddr.sun_family = AF_LOCAL;
    StrMov(oSvrSock.stUNIXAddr.sun_path, pszSockPath); // "/tmp/pipe_channel.sock"
    //strcpy(oSvrSock.stUNIXAddr.sun_path, pszSockPath);

    if(bBind)
    {
        const int on = 1;
        setsockopt(oSvrSock.iSockfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

        unlink(pszSockPath); // unlink it if exist
        
        if ((::bind(
            oSvrSock.iSockfd, 
            (struct sockaddr *)&(oSvrSock.stUNIXAddr),
            SUN_LEN(&(oSvrSock.stUNIXAddr)))) == -1)
            //strlen(oSvrSock.stUNIXAddr.sun_path)+sizeof(oSvrSock.stUNIXAddr.sun_family))) == -1)
        {
            close(oSvrSock.iSockfd);
            std::stringstream oss;
            oss << "[CNetServerReactor::RegisterUSockUDPServer] bind socket failed";
            oss << ", sun_path:" << oSvrSock.stUNIXAddr.sun_path;
            oss << ", errno:" << errno << ", strerror:" << strerror(errno);
            m_sLastErrMsg = oss.str();
            return -1;
        }

        //int iSocketOptReceiveBufferSize = 40 * 1024 * 1024;
        int iSocketOptReceiveBufferSize = 2;
        setsockopt(oSvrSock.iSockfd, SOL_SOCKET, SO_RCVBUF, (char*)&iSocketOptReceiveBufferSize,  sizeof(iSocketOptReceiveBufferSize));
    }
    else
    {
        int iSocketOptSendBufferSize = 2;
        setsockopt(oSvrSock.iSockfd, SOL_SOCKET, SO_SNDBUF, (char*)&iSocketOptSendBufferSize,  sizeof(iSocketOptSendBufferSize));
    }

    umask(old_mod); // Set back umask

    // Set NO DELAY
    if(bNoDelay)
    {
        int iCurrentFlag = 0;
        if((iCurrentFlag = fcntl(oSvrSock.iSockfd, F_GETFL, 0)) == -1)
        {
            close(oSvrSock.iSockfd);
            m_sLastErrMsg = "[CNetServerReactor::RegisterUSockUDPServer] fcntl F_GETFL failed";
            return -1;
        }

        if (fcntl(oSvrSock.iSockfd, F_SETFL, iCurrentFlag | FNDELAY) == -1)
        {
            close(oSvrSock.iSockfd);
            m_sLastErrMsg = "[CNetServerReactor::RegisterUSockUDPServer] fcntl F_SETFL failed";
            return -1;
        }
    }

    int flags = 0;
	flags = fcntl(oSvrSock.iSockfd, F_GETFL, 0);
    if (flags != -1)
    {
	    fcntl(oSvrSock.iSockfd, F_SETFL, flags | O_NONBLOCK);
    }

    m_oUDPSvrSockInfoMap[m_iCurrentUDPSvrSockIndex] = oSvrSock;
    iServerHandle = m_iCurrentUDPSvrSockIndex++;

    if(oSvrSock.iSockfd >= m_iNumOfFds)
        m_iNumOfFds = oSvrSock.iSockfd + 1;

	if(bFDWatch)
		FD_SET(oSvrSock.iSockfd, &m_setReadFdsHolder);

    return 0;
}

int CNetServerReactor::RegisterUSockUDPPeerAddr(
    int& iPeerHandle,   // [OUT]
    const char* pszSockPath)
{
    if(!m_bNeedUDP)
    {
        m_sLastErrMsg = "[CNetServerReactor::RegisterUSockUDPPeerAddr] m_bNeedUDP = false";
        return -1;
    }

    // Make Peer Addr
    struct sockaddr_un stUNIXAddr;
    memset(&stUNIXAddr, 0, sizeof(stUNIXAddr));
    stUNIXAddr.sun_family = AF_UNIX;
    StrMov(stUNIXAddr.sun_path, pszSockPath); // "/tmp/pipe_channel.sock"

    m_oUSockUDPPeerAddrInfoMap[m_iCurrentUDPPeerAddrIndex] = stUNIXAddr;
    iPeerHandle = m_iCurrentUDPPeerAddrIndex++;

    return 0;
}

int CNetServerReactor::USockUDPSendTo(
                                      int iServerHandle,
                                      const char* pSendBuf, 
                                      uint32_t nBufLen, 
                                      int iPeerHandle,
                                      int iFlag)
{
    if(!m_bNeedUDP)
    {
        m_sLastErrMsg = "[CNetServerReactor::USockUDPSendTo] m_bNeedUDP = false";
        return -1;
    }

    // Check Server Handle
    std::map<int, UDPServerSockInfo_T>::const_iterator ItSvr 
        = m_oUDPSvrSockInfoMap.find(iServerHandle);

    if(ItSvr == m_oUDPSvrSockInfoMap.end())
    {
        m_sLastErrMsg = "[CNetServerReactor::USockUDPSendTo] unregisted sever handle. iServerHandle = ";
        std::ostringstream oss;
        oss << iServerHandle;
        m_sLastErrMsg += oss.str();

        return -1;
    }

    // Check Peer Handle
    std::map<int, sockaddr_un>::const_iterator ItPeer
        = m_oUSockUDPPeerAddrInfoMap.find(iPeerHandle);

    if(ItPeer == m_oUSockUDPPeerAddrInfoMap.end())
    {
        m_sLastErrMsg = "[CNetServerReactor::USockUDPSendTo] unregisted peer handle. iPeerHandle = ";
        std::ostringstream oss;
        oss << iPeerHandle;
        m_sLastErrMsg += oss.str();

        return -1;
    }

    // Send Buffer
    int iBytesSent = ::sendto(
        (*ItSvr).second.iSockfd,
        pSendBuf,
        nBufLen,
        iFlag,
        (struct sockaddr *)&((*ItPeer).second),
        sizeof(struct sockaddr_un));

    if(iBytesSent == -1 || static_cast<uint32_t>(iBytesSent) != nBufLen)
    {
        m_sLastErrMsg = "[CNetServerReactor::RegisterUSockUDPServer] sendto peer failed. errno = ";
        std::ostringstream oss1;
        oss1 << errno;
        m_sLastErrMsg += oss1.str();

        return -1;
    }

    return 0;
}

int CNetServerReactor::USockUDPSendTo(
                                      int iServerHandle,
                                      const char* pSendBuf, 
                                      uint32_t nBufLen, 
                                      const char* pszSockPath,
                                      int iFlag)
{
    if(!m_bNeedUDP)
    {
        m_sLastErrMsg = "[CNetServerReactor::USockUDPSendTo] m_bNeedUDP = false";
        return -1;
    }

    // Check Server Handle
    std::map<int, UDPServerSockInfo_T>::const_iterator ItSvr 
        = m_oUDPSvrSockInfoMap.find(iServerHandle);

    if(ItSvr == m_oUDPSvrSockInfoMap.end())
    {
        m_sLastErrMsg = "[CNetServerReactor::USockUDPSendTo] unregisted sever handle. iServerHandle = ";
        std::ostringstream oss;
        oss << iServerHandle;
        m_sLastErrMsg += oss.str();

        return -1;
    }

    // Make Peer Addr
    struct sockaddr_un stUNIXAddr;
    memset(&stUNIXAddr, 0, sizeof(stUNIXAddr));
    stUNIXAddr.sun_family = AF_UNIX;
    StrMov(stUNIXAddr.sun_path, pszSockPath); // "/tmp/pipe_channel.sock"

    // Send Buffer
    int iBytesSent = ::sendto(
        (*ItSvr).second.iSockfd,
        pSendBuf,
        nBufLen,
        iFlag,
        (struct sockaddr *)&(stUNIXAddr),
        sizeof(struct sockaddr_un));

    if(iBytesSent == -1 || static_cast<uint32_t>(iBytesSent) != nBufLen)
    {
        m_sLastErrMsg = "[CNetServerReactor::USockUDPSendTo] sendto peer failed. errno = ";
        std::ostringstream oss1;
        oss1 << errno << ", strerror:" << strerror(errno);
        m_sLastErrMsg += oss1.str();

        return -1;
    }

    return 0;
}

int CNetServerReactor::USockUDPSendTo(
                                      const char* pSendBuf, 
                                      uint32_t nBufLen, 
                                      const char* pszSockPath,
                                      int iFlag)
{
    if(!m_bNeedUDP)
    {
        m_sLastErrMsg = "[CNetServerReactor::USockUDPSendTo] m_bNeedUDP = false";
        return -1;
    }

    int iSockfd = ::socket(AF_UNIX, SOCK_DGRAM, 0);
    if(iSockfd < 0 )
    {
        m_sLastErrMsg = "[CNetServerReactor::USockUDPSendTo] create socket failed";
        return -1;
    }

    // Make Peer Addr
    struct sockaddr_un stUNIXAddr;
    memset(&stUNIXAddr, 0, sizeof(stUNIXAddr));
    stUNIXAddr.sun_family = AF_UNIX;
    StrMov(stUNIXAddr.sun_path, pszSockPath); // "/tmp/pipe_channel.sock"

    // Send Buffer
    int iBytesSent = ::sendto(
        iSockfd,
        pSendBuf,
        nBufLen,
        iFlag,
        (struct sockaddr *)&(stUNIXAddr),
        sizeof(struct sockaddr_un));

    if(iBytesSent == -1 || static_cast<uint32_t>(iBytesSent) != nBufLen)
    {
        m_sLastErrMsg = "[CNetServerReactor::USockUDPSendTo] sendto peer failed. errno = ";
        std::ostringstream oss1;
        oss1 << errno << ", strerror:" << strerror(errno);
        m_sLastErrMsg += oss1.str();

        return -1;
    }

    return 0;
}

char* CNetServerReactor::StrMov(
                                register char *dst, 
                                register const char *src)
{
    while((*dst++ = *src++)) ;

    return dst-1;
}

uint32_t CNetServerReactor::CleanTcpSvrHandle(int iBeginHandle, int iStep)
{
	// Unsupported
	return 0;
}

void CNetServerReactor::FDWatchServer(
		int iTcpServerHandle, 
		int iUdpServerHandle, 
		int iUSockTcpServerHandle, 
		int iUSockUdpServerHandle)
{
	if(iTcpServerHandle >= 0)
		FD_SET(m_oTCPSvrSockInfoMap[iTcpServerHandle].iSockfd, &m_setReadFdsHolder);

	if(iUdpServerHandle >= 0)
		FD_SET(m_oUDPSvrSockInfoMap[iUdpServerHandle].iSockfd, &m_setReadFdsHolder);

	if(iUSockTcpServerHandle >= 0)
		FD_SET(m_oTCPSvrSockInfoMap[iUSockTcpServerHandle].iSockfd, &m_setReadFdsHolder);

	if(iUSockUdpServerHandle >= 0)
		FD_SET(m_oUDPSvrSockInfoMap[iUSockUdpServerHandle].iSockfd, &m_setReadFdsHolder);
}

