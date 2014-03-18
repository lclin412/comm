
#include <errno.h>
#include <assert.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <string.h>

#include <algorithm>
#include <iostream>
#include <string>
#include <iostream>
#include <sstream>
#include "epoll_svr_reactor.h"
//#include "mutex.h"
//#include <sys/types.h>
//#include <sys/stat.h>
//#include <fcntl.h>


const long DEFAULT_EPOLL_WAIT_MSEC = 10;
const long DEFAULT_SLEEP_USEC = 10000;

#define IS_ERR_HUP(ulEvents)  ((ulEvents & (EPOLLERR | EPOLLHUP)) != 0)
#define IS_READ_AVAIL(ulEvents)  ((ulEvents & (EPOLLIN)) != 0)
#define IS_WRITE_AVAIL(ulEvents) ((ulEvents & (EPOLLOUT)) != 0)

CEpollServerReactor::CEpollServerReactor()
: m_epfd(-1)
, m_aEpollEvents(NULL)
, m_nEvents(0)
, m_iCurrentUDPSvrSockIndex(1)
, m_iCurrentUDPPeerAddrIndex(1)
, m_pUDPNetHandler(NULL)
, m_iCurrentTCPSvrSockIndex(1)
, m_iCurrentTimerIndex(1)
, m_bEventStop(true)
, m_bNeedTimer(true)
, m_bNeedUDP(true)
, m_bNeedTCPServer(true)
, m_bNeedTCPServerAccept(true)
, m_bNeedTCPClient(true)
, m_bNeedUserAction(true)
{
    m_l_iFromAddrLen = sizeof(struct sockaddr_in);
	memset(m_arrTCPNetHandler, 0, sizeof(m_arrTCPNetHandler));
}

CEpollServerReactor::~CEpollServerReactor()
{
    // Close all udp server sockets
    for(m_l_udp_svr_it = m_oUDPSvrSockInfoMap.begin()
        ; m_l_udp_svr_it != m_oUDPSvrSockInfoMap.end()
        ; m_l_udp_svr_it++)
    {
        ::close((*m_l_udp_svr_it).second.iSockfd);
    } 

	this->CleanTcpSvrHandle(0, MAX_EPOLL_EVENT_NUM);
    
    ::close(m_epfd);

    if(m_aEpollEvents)
        delete[] m_aEpollEvents;
}

int CEpollServerReactor::Initialize(
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
int CEpollServerReactor::RegisterUDPServer(
                                         int& iServerHandle, // [OUT]
                                         unsigned short ushPort, 
                                         const char* pszHost,
                                         bool bNoDelay,
										 bool bFDWatch)
{
    if(!m_bNeedUDP)
    {
        m_sLastErrMsg = "[CEpollServerReactor::RegisterUDPServer] m_bNeedUDP = false";
        return -1;
    }
    
    CUDPSockInfo oSvrSock;
    
    struct hostent* l_pHostInfo = NULL;
    struct in_addr* l_pAddp = NULL;
    
    if(pszHost) 
    {
        l_pHostInfo = ::gethostbyname(pszHost);
        if(l_pHostInfo == NULL)
        {
            m_sLastErrMsg = "[CEpollServerReactor::RegisterUDPServer] gethostbyname failed. pszHost = ";
            m_sLastErrMsg += pszHost;
            return -1;
        }
    }
    
    oSvrSock.iSockfd = ::socket(AF_INET, SOCK_DGRAM, 0);
    if(oSvrSock.iSockfd < 0 )
    {
        m_sLastErrMsg = "[CEpollServerReactor::RegisterUDPServer] create socket failed";
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
        m_sLastErrMsg = "[CEpollServerReactor::RegisterUDPServer] bind socket failed";
        return -1;
    }
    
    // Set NO DELAY
    if(bNoDelay)
    {
        int iCurrentFlag = 0;
        if((iCurrentFlag = fcntl(oSvrSock.iSockfd, F_GETFL, 0)) == -1)
        {
            close(oSvrSock.iSockfd);
            m_sLastErrMsg = "[CEpollServerReactor::RegisterUDPServer] fcntl F_GETFL failed";
            return -1;
        }
        
        if (fcntl(oSvrSock.iSockfd, F_SETFL, iCurrentFlag | FNDELAY) == -1)
        {
            close(oSvrSock.iSockfd);
            m_sLastErrMsg = "[CEpollServerReactor::RegisterServer] fcntl F_SETFL failed";
            return -1;
        }
    }

	int iSocketOptSendBufferSize = 40 * 1024 * 1024;
	int iSocketOptReceiveBufferSize = 40 * 1024 * 1024;
	setsockopt(oSvrSock.iSockfd, SOL_SOCKET, SO_SNDBUF, (char*)&iSocketOptSendBufferSize,  sizeof(iSocketOptSendBufferSize));	
	setsockopt(oSvrSock.iSockfd, SOL_SOCKET, SO_RCVBUF, (char*)&iSocketOptReceiveBufferSize,  sizeof(iSocketOptReceiveBufferSize));
    
    m_oUDPSvrSockInfoMap[oSvrSock.iSockfd] = oSvrSock;
    iServerHandle = oSvrSock.iSockfd;

	//fprintf(stderr, "pid=%d, bind UDP handle=%d\n",getpid(),oSvrSock.iSockfd);
    
	if(bFDWatch)
	{
		// Set epoll flags
		FdAdd_i(oSvrSock.iSockfd, UDP_EVENT_FLAG);
	}

    return 0;
}

void CEpollServerReactor::FDWatchServer(
		int iTcpServerHandle, 
		int iUdpServerHandle, 
		int iUSockTcpServerHandle, 
		int iUSockUdpServerHandle)
{
	if(iTcpServerHandle >= 0)
		FdAdd_i(m_arrTCPSvrSockInfo[iTcpServerHandle].iSockfd, TCP_SERVER_ACCEPT_EVENT_FLAG, &(m_arrTCPSvrSockInfo[iTcpServerHandle]));

	if(iUSockTcpServerHandle >= 0)
		FdAdd_i(m_arrTCPSvrSockInfo[iUSockTcpServerHandle].iSockfd, TCP_SERVER_ACCEPT_EVENT_FLAG, &(m_arrTCPSvrSockInfo[iUSockTcpServerHandle]));

	if(iUdpServerHandle >= 0)
		FdAdd_i(iUdpServerHandle, UDP_EVENT_FLAG);

	if(iUSockUdpServerHandle >= 0)
		FdAdd_i(iUSockUdpServerHandle, UDP_EVENT_FLAG);
}

int CEpollServerReactor::RegisterPeerAddr(
                                        int& iPeerHandle,   // [OUT]
                                        const char* pszHost,
                                        unsigned short ushPort)
{
    if(!m_bNeedUDP)
    {
        m_sLastErrMsg = "[CEpollServerReactor::RegisterPeerAddr] m_bNeedUDP = false";
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

int CEpollServerReactor::SendTo(
                              int iServerHandle,
                              const char* pSendBuf, 
                              uint32_t nBufLen, 
                              int iPeerHandle,
                              int iFlag)
{
    if(!m_bNeedUDP)
    {
        m_sLastErrMsg = "[CEpollServerReactor::SendTo] m_bNeedUDP = false";
        return -1;
    }
    
    // Check Server Handle
    hash_map<int, UDPSockInfo_T, hash<int> >::const_iterator ItSvr 
        = m_oUDPSvrSockInfoMap.find(iServerHandle);
    
    if(ItSvr == m_oUDPSvrSockInfoMap.end())
    {
        m_sLastErrMsg = "[CEpollServerReactor::SendTo] unregisted sever handle. iServerHandle = ";
        std::ostringstream oss;
        oss << iServerHandle;
        m_sLastErrMsg += oss.str();
        
        return -1;
    }
    
    // Check Peer Handle
    hash_map<int, sockaddr_in, hash<int> >::const_iterator ItPeer
        = m_oUDPPeerAddrInfoMap.find(iPeerHandle);
    
    if(ItPeer == m_oUDPPeerAddrInfoMap.end())
    {
        m_sLastErrMsg = "[CEpollServerReactor::SendTo] unregisted peer handle. iPeerHandle = ";
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
        m_sLastErrMsg = "[CEpollServerReactor::SendTo] sendto peer failed. errno = ";
        std::ostringstream oss1;
        oss1 << errno;
        m_sLastErrMsg += oss1.str();
        
        return -1;
    }
    
    return 0;
}

int CEpollServerReactor::SendTo(
                              int iServerHandle,
                              const char* pSendBuf, 
                              uint32_t nBufLen, 
                              const char* pszHost,
                              unsigned short ushPort,
                              int iFlag)
{
    if(!m_bNeedUDP)
    {
        m_sLastErrMsg = "[CEpollServerReactor::SendTo] m_bNeedUDP = false";
        return -1;
    }
    
    // Check Server Handle
    hash_map<int, UDPSockInfo_T, hash<int> >::const_iterator ItSvr 
        = m_oUDPSvrSockInfoMap.find(iServerHandle);
    
    if(ItSvr == m_oUDPSvrSockInfoMap.end())
    {
        m_sLastErrMsg = "[CEpollServerReactor::SendTo] unregisted sever handle. iServerHandle = ";
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
    
    if(iBytesSent < 0 || static_cast<uint32_t>(iBytesSent) != nBufLen)
    {
        m_sLastErrMsg = "[CEpollServerReactor::SendTo] sendto peer failed. errno = ";
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
int CEpollServerReactor::RegisterTCPServer(
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
        m_sLastErrMsg = "[CEpollServerReactor::RegisterTCPServer] m_bNeedTCPServer = false";
        return -1;
    }
    
    assert(pHandler != NULL);
    
    CTCPSockInfo oSvrSock;
    
    struct hostent* l_pHostInfo = NULL;
    struct in_addr* l_pAddp = NULL;
    
    if(pszHost) 
    {
        l_pHostInfo = ::gethostbyname(pszHost);
        if(l_pHostInfo == NULL)
        {
            m_sLastErrMsg = "[CEpollServerReactor::RegisterTCPServer] gethostbyname failed. pszHost = ";
            m_sLastErrMsg += pszHost;
            return -1;
        }
    }
    
    oSvrSock.iSockfd = ::socket(AF_INET, SOCK_STREAM, 0);
    if(oSvrSock.iSockfd < 0 )
    {
        m_sLastErrMsg = "[CEpollServerReactor::RegisterTCPServer] create socket failed";
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

    int keepAlive = 1, keepInterval = 30, keepCount = 3, keepidle = 120;
    if (setsockopt(oSvrSock.iSockfd, SOL_SOCKET, SO_KEEPALIVE, &keepAlive, sizeof(int)) < 0)
    {
       fprintf(stderr, "setsockopt SO_KEEPALIVE failed, %s\n", strerror(errno));

       close(oSvrSock.iSockfd);
       return -1;
    }

    if (setsockopt(oSvrSock.iSockfd, SOL_TCP, TCP_KEEPIDLE, &keepidle, sizeof(int)) < 0)
    {
       fprintf(stderr, "setsockopt SO_KEEPIDLE failed, %s\n", strerror(errno));

       close(oSvrSock.iSockfd);
       return -1;
    }

    if (setsockopt(oSvrSock.iSockfd, SOL_TCP, TCP_KEEPINTVL, &keepInterval, sizeof(int)) < 0)
    {
       fprintf(stderr, "setsockopt TCP_KEEPINTVL failed\n");

       close(oSvrSock.iSockfd);
       return -1;
    }

    if (setsockopt(oSvrSock.iSockfd, SOL_TCP, TCP_KEEPCNT, &keepCount, sizeof(int)) < 0)
    {
       fprintf(stderr, "setsockopt TCP_KEEPCNT failed\n");

       close(oSvrSock.iSockfd);
       return -1;
    }


    if ((::bind(
        oSvrSock.iSockfd, 
        (struct sockaddr *)&(oSvrSock.stINETAddr),
        sizeof(struct sockaddr))) == -1) 
    {
        close(oSvrSock.iSockfd);
        m_sLastErrMsg = "[CEpollServerReactor::RegisterTCPServer] bind socket failed";
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
            m_sLastErrMsg = "[CEpollServerReactor::RegisterTCPServer] fcntl F_GETFL failed";
            return -1;
        }
       
        if (fcntl(oSvrSock.iSockfd, F_SETFL, iCurrentFlag | FNDELAY) == -1)
        {
            close(oSvrSock.iSockfd);
            m_sLastErrMsg = "[CEpollServerReactor::RegisterTCPServer] fcntl F_SETFL failed";
            return -1;
        }
    }
    
    int iBacklog = 1024;   // Max pending connection
    if(listen(oSvrSock.iSockfd, iBacklog) < 0)
    {
        close(oSvrSock.iSockfd);
        m_sLastErrMsg = "[CEpollServerReactor::RegisterTCPServer] listen failed";
        return -1;
    }

	//fprintf(stderr, "pid=%d, listen tcp iSockfd=%d\n", getpid(),oSvrSock.iSockfd);
    
    oSvrSock.iRcvBufLen = iRcvBufLen;
    
    m_arrTCPSvrSockInfo[m_iCurrentTCPSvrSockIndex] = oSvrSock;
    oSvrSock.iTCPSvrSockIndex = m_iCurrentTCPSvrSockIndex;

	if(bFDWatch)
	{
		// Set epoll flags
		FdAdd_i(oSvrSock.iSockfd, TCP_SERVER_ACCEPT_EVENT_FLAG, &(m_arrTCPSvrSockInfo[m_iCurrentTCPSvrSockIndex]));
	}

	iServerHandle = m_iCurrentTCPSvrSockIndex++;
#ifdef __REACTOR_FUTEX_SWITCH__
    //linux下NPTL的线程同步封装了futex锁
    {
        pthread_mutexattr_t mattr ;
        int fd = -1 ;
        
        fd = open("/dev/zero",O_RDWR,0) ;
        m_pMutex = (pthread_mutex_t *)mmap(0,sizeof(pthread_mutex_t),PROT_READ|PROT_WRITE,MAP_SHARED,fd,0) ;
        close(fd) ;
        
        pthread_mutexattr_init(&mattr) ;
        pthread_mutexattr_setpshared(&mattr,PTHREAD_PROCESS_SHARED);
        pthread_mutex_init(m_pMutex,&mattr) ;
    }
#endif
    
    return 0;
}

int CEpollServerReactor::ConnectToServer(
                                       ITCPNetHandler* pHandler,
                                       unsigned short ushServerPort, 
                                       const char* pszServerHost,
				       uint32_t dwConnTimeout
                                       )
{
    dwConnTimeout = (dwConnTimeout > 0 ? dwConnTimeout : 10);
    
    if(!m_bNeedTCPClient)
    {
        m_sLastErrMsg = "[CEpollServerReactor::ConnectToServer] m_bNeedTCPClient = false";
        return -1;
    }
    
    assert(pHandler != NULL);
    
    CTCPSockInfo oSvrSock;
    
    struct hostent* l_pHostInfo = NULL;
    struct in_addr* l_pAddp = NULL;
    
    if(pszServerHost) 
    {
        l_pHostInfo = ::gethostbyname(pszServerHost);
        if(l_pHostInfo == NULL)
        {
            m_sLastErrMsg = "[CEpollServerReactor::ConnectToServer] gethostbyname failed. pszServerHost = ";
            m_sLastErrMsg += pszServerHost;
            return -1;
        }
    }
    
    oSvrSock.iSockfd = ::socket(AF_INET, SOCK_STREAM, 0);
    if(oSvrSock.iSockfd < 0 )
    {
        m_sLastErrMsg = "[CEpollServerReactor::ConnectToServer] create socket failed";
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
	    m_sLastErrMsg = "[CEpollServerReactor::ConnectToServer] connect to server ";
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
	    m_sLastErrMsg = "[CEpollServerReactor::ConnectToServer] connect to server ";
	    m_sLastErrMsg += pszServerHost;
	    m_sLastErrMsg += " failed";

	    close(oSvrSock.iSockfd);
	    return -1;
	}

	if(!FD_ISSET(oSvrSock.iSockfd, &stWtFds))
	{
	    m_sLastErrMsg = "[CEpollServerReactor::ConnectToServer] connect to server ";
	    m_sLastErrMsg += pszServerHost;
	    m_sLastErrMsg += " timeout";

	    close(oSvrSock.iSockfd);
	    return -1;
	}	

	int error = 0;
	socklen_t len = sizeof(error);
	if(getsockopt(oSvrSock.iSockfd, SOL_SOCKET, SO_ERROR, &error, &len) < 0 || error)
	{
	    m_sLastErrMsg = "[CEpollServerReactor::ConnectToServer] connect to server ";
	    m_sLastErrMsg += pszServerHost;
	    m_sLastErrMsg += " failed";

	    close(oSvrSock.iSockfd);
	    return -1;
	}
    }

    int iCurrentFlag = 0;
    if((iCurrentFlag = fcntl(oSvrSock.iSockfd, F_GETFL, 0)) == -1)
    {
	close(oSvrSock.iSockfd);
	m_sLastErrMsg = "[CEpollServerReactor::ConnectToServer] fcntl F_GETFL failed";
	return -1;
    }

    if (fcntl(oSvrSock.iSockfd, F_SETFL, iCurrentFlag | FNDELAY) == -1)
    {
	close(oSvrSock.iSockfd);
	m_sLastErrMsg = "[CEpollServerReactor::ConnectToServer] fcntl F_SETFL failed";
	return -1;
    }
    
	m_arrTCPCliConnHandle[oSvrSock.iSockfd] = oSvrSock;

    // Set epoll flags
    FdAdd_i(oSvrSock.iSockfd, TCP_CLIENT_EVENT_FLAG, &(m_arrTCPCliConnHandle[oSvrSock.iSockfd]));
    
    // Callback
    pHandler->OnConnect(
        oSvrSock.iSockfd,  // ConnHandler
        (oSvrSock.stINETAddr.sin_addr.s_addr),
        ushServerPort,
        0);
    
    return 0;
}

int CEpollServerReactor::Send(
                               int iConnHandle,
                               char* pSendBuf, 
                               uint32_t nBufLen, 
                               int iFlag)
{
    if(!m_bNeedTCPServer && !m_bNeedTCPClient)
    {
        m_sLastErrMsg = "[CEpollServerReactor::Send] !m_bNeedTCPServer && !m_bNeedTCPClient";
        return 0;
    }    
    
    // Send Buffer
    int iBytesSent = ::send(
        iConnHandle,
        pSendBuf,
        nBufLen,
        iFlag);
    
    if(iBytesSent < 0)
    {
		if(errno != EINTR && errno != EAGAIN && errno != EWOULDBLOCK)
		{
			m_sLastErrMsg = "[CEpollServerReactor::Send] sendto peer failed. errno = ";
			std::ostringstream oss1;
			oss1 << errno;
			m_sLastErrMsg += oss1.str();
			return -1;
		}
        
        return 0;
    }
    
    return iBytesSent;
}

int CEpollServerReactor::Close(int iConnHandle, bool bAsServer)
{
    if(!m_bNeedTCPServer && !m_bNeedTCPClient)
    {
        m_sLastErrMsg = "[CEpollServerReactor::Close] !m_bNeedTCPServer && !m_bNeedTCPClient";
        return -1;
    }  
    
    FdDel_i(iConnHandle);

    ::close(iConnHandle);
    
    if(bAsServer)
    {
        // Parallel server, one connection -- one Process
        // Shutdown this Process when connection down.
        if(m_arrTCPSvrSockInfo[m_arrTCPSvrConnHandle[iConnHandle]].bParallel)
        {
            m_bEventStop = true;
        }
    }
    
    return 0;
}

void CEpollServerReactor::NeedOnSendCheck(int iConnHandle)
{
    FdMod_i(iConnHandle, true);
}
/////////////////// Timer ///////////////////
int CEpollServerReactor::RegisterTimer(
                                     int& iTimerID,      // [OUT]
                                     ITimerHandler* pHandler,
                                     const CTimeValue& tvInterval,
                                     unsigned int dwCount)
{
    if(!m_bNeedTimer)
    {
        m_sLastErrMsg = "[CEpollServerReactor::RegisterTimer] m_bNeedTimer = false";
        return -1;
    } 
    
    int iRetCode = m_oTimerQueue.ScheduleTimer(
        pHandler, 
        ++m_iCurrentTimerIndex, 
        tvInterval, 
        dwCount);
    
    if(iRetCode != 0)
    {
        m_sLastErrMsg = "[CEpollServerReactor::RegisterTimer] ScheduleTimer failed";
        return -1;
    }
    
    iTimerID = m_iCurrentTimerIndex;
    
    return 0;
}

int CEpollServerReactor::UnregisterTimer(int iTimerID)
{
    if(!m_bNeedTimer)
    {
        m_sLastErrMsg = "[CEpollServerReactor::UnregisterTimer] m_bNeedTimer = false";
        return -1;
    } 
    
    int iRetCode = m_oTimerQueue.CancelTimer(iTimerID);
    if(iRetCode != 0)
    {
        m_sLastErrMsg = "[CEpollServerReactor::UnregisterTimer] CancelTimer failed, iTimerID = ";
        return -1;
    }
    
    return 0;
}

////////////// Register Event Handler ///////////////
void CEpollServerReactor::RegisterUDPNetHandler(
                                              IUDPNetHandler* pHandler)
{
    if(!m_bNeedUDP)
    {
        m_sLastErrMsg = "[CEpollServerReactor::RegisterUDPNetHandler] m_bNeedUDP = false";
        return;
    } 
    
    m_pUDPNetHandler = pHandler;
}

void CEpollServerReactor::RegisterUserEventHandler(
                                                 IUserEventHandler* pUserEventHandler
                                                 )
{
    if(!m_bNeedUserAction)
    {
        m_sLastErrMsg = "[CEpollServerReactor::RegisterUserEventHandler] m_bNeedUserAction = false";
        return;
    } 
    
    m_pUserEventHandler = pUserEventHandler;
}

/////////////////// Run Event Loop///////////////////
void CEpollServerReactor::RunEventLoop()
{
    m_bEventStop = false;
    
    while(!m_bEventStop)
    {
        m_iEcc = this->CheckEvents();
        if(0 == m_iEcc)		
            continue;
        
        this->ProcessSocketEvent();
    }
}

void CEpollServerReactor::StopEventLoop()
{
    m_bEventStop = true;
}

int CEpollServerReactor::CheckEvents()
{
	// Check User Event
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

    // Check Socket Status
	//memset(m_aEpollEvents, 0, MAX_EPOLL_EVENT_NUM * sizeof(epoll_event));
	if(m_epfd != -1)
	{
#ifdef __REACTOR_FUTEX_SWITCH__
        //pthread_mutex_lock(m_pMutex);
#endif
		m_iEcc = ::epoll_wait(m_epfd, m_aEpollEvents, m_nEvents, DEFAULT_EPOLL_WAIT_MSEC);
#ifdef __REACTOR_FUTEX_SWITCH__
        //pthread_mutex_unlock(m_pMutex);
#endif
	}	
	else // sleep for a while
	{
		struct timeval tv;
		tv.tv_sec = 0;
		tv.tv_usec = DEFAULT_SLEEP_USEC;
		select(0, NULL, NULL, NULL, &tv);
		m_iEcc = 0;
	}
    
    return m_iEcc;
}

int CEpollServerReactor::ProcessSocketEvent()
{
    static int iHandle = 0;
    static int j = 0;

    if (m_iEcc > 0)
    {
        // Set Fd set flags
        for(j = 0; j < m_iEcc; ++j)
        {
            assert(m_aEpollEvents[j].data.ptr);
            SelItem_T& rSelItem = *(reinterpret_cast<SelItem_T*>(m_aEpollEvents[j].data.ptr));
            iHandle = rSelItem.iHandle;
			//fprintf(stderr, "pid = %d, m_iEcc = %d, iHandle = %d, Flag = 0x%x\n", getpid(), m_iEcc, iHandle, rSelItem.enEventFlag);

            switch(rSelItem.enEventFlag) 
            {
            case TCP_CLIENT_EVENT_FLAG:
                {
                    if(m_bNeedTCPClient)
                    {
                        assert(rSelItem.pvParam);
                        TCPSockInfo_T& rTcpCliSockInfo = *(reinterpret_cast<TCPSockInfo_T*>(rSelItem.pvParam));
                        this->ProcessTcpCliEvent_i(iHandle, m_aEpollEvents[j].events, rTcpCliSockInfo);
                    }
                }
                break;
            case TCP_SERVER_EVENT_FLAG:
                {
                    if(m_bNeedTCPServer)
                    {
                        assert(rSelItem.pvParam);
                        TCPSockInfo_T& rTcpSvrSockInfo = *(reinterpret_cast<TCPSockInfo_T*>(rSelItem.pvParam));
                        this->ProcessTcpSvrEvent_i(iHandle, m_aEpollEvents[j].events, rTcpSvrSockInfo);
                    }
                }
                break;
            case TCP_SERVER_ACCEPT_EVENT_FLAG:
                {
                    if(m_bNeedTCPServer)
                    {
                        assert(rSelItem.pvParam);
                        TCPSockInfo_T& rTcpSockInfo = *(reinterpret_cast<TCPSockInfo_T*>(rSelItem.pvParam));
                        this->CheckTCPComingConnection_i(rTcpSockInfo);
                    }
                }
                break;
            case UDP_EVENT_FLAG:
                {
                    if(m_bNeedUDP)
                        this->ProcessUDPEvent_i(iHandle);
                }
                break;
            default:
				fprintf(stderr, "pid = %d, Unknow Event Flag = 0x%x, iHandle = %d\n", getpid(), rSelItem.enEventFlag, iHandle);
                assert(0);
            }
        }
    }

    return 0;
}

const char* CEpollServerReactor::GetLastErrMsg() const
{
    return m_sLastErrMsg.c_str();
}

int CEpollServerReactor::RegisterUSockTCPServer(
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
        m_sLastErrMsg = "[CEpollServerReactor::RegisterUSockServer] m_bNeedTCPServer = false";
        return -1;
    }
    
    assert(pszSockPath != NULL);
    assert(pHandler != NULL);
    
    int iRetCode = 0;
    CTCPSockInfo oSvrSock;
    
    oSvrSock.iSockfd = ::socket(AF_UNIX, SOCK_STREAM, 0);
    if(oSvrSock.iSockfd < 0 )
    {
        m_sLastErrMsg = "[CEpollServerReactor::RegisterUSockServer] create socket failed";
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
        m_sLastErrMsg = "[CEpollServerReactor::RegisterUSockServer] bind socket failed";
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
            m_sLastErrMsg = "[CEpollServerReactor::RegisterUSockServer] Set Socket Linger failed";
            return -1;
        }
        
        int iCurrentFlag = 0;
        if((iCurrentFlag = fcntl(oSvrSock.iSockfd, F_GETFL, 0)) == -1)
        {
            close(oSvrSock.iSockfd);
            m_sLastErrMsg = "[CEpollServerReactor::RegisterUSockServer] fcntl F_GETFL failed";
            return -1;
        }
        
        if (fcntl(oSvrSock.iSockfd, F_SETFL, iCurrentFlag | FNDELAY) == -1)
        {
            close(oSvrSock.iSockfd);
            m_sLastErrMsg = "[CEpollServerReactor::RegisterUSockServer] fcntl F_SETFL failed";
            return -1;
        }
    }
    
    int iBacklog = 5;   // Max pending connection
    if(listen(oSvrSock.iSockfd, iBacklog) < 0)
    {
        close(oSvrSock.iSockfd);
        m_sLastErrMsg = "[CEpollServerReactor::RegisterUSockServer] listen failed";
        return -1;
    }
    
    oSvrSock.iRcvBufLen = iRcvBufLen;
    
    m_arrTCPSvrSockInfo[m_iCurrentTCPSvrSockIndex] = oSvrSock;

	if(bFDWatch)
	{
		// Set epoll flags
		FdAdd_i(oSvrSock.iSockfd, TCP_SERVER_ACCEPT_EVENT_FLAG, &(m_arrTCPSvrSockInfo[m_iCurrentTCPSvrSockIndex]));
	}

    iServerHandle = m_iCurrentTCPSvrSockIndex++;
    
    return 0;
}

int CEpollServerReactor::ConnectToUSockTCPServer(
                                            ITCPNetHandler* pHandler,
                                            const char* pszSockPath,
					    uint32_t dwConnTimeout
                                            )
{
    if(!m_bNeedTCPClient)
    {
        m_sLastErrMsg = "[CEpollServerReactor::ConnectToUSockServer] m_bNeedTCPClient = false";
        return -1;
    }
    
    assert(pszSockPath != NULL);
    assert(pHandler != NULL);
    
    CTCPSockInfo oSvrSock;
    
    oSvrSock.iSockfd = ::socket(AF_UNIX, SOCK_STREAM, 0);
    if(oSvrSock.iSockfd < 0 )
    {
        m_sLastErrMsg = "[CEpollServerReactor::ConnectToUSockServer] create socket failed";
        return -1;
    }
    
    memset(&oSvrSock.stUNIXAddr, 0, sizeof(oSvrSock.stUNIXAddr));
    oSvrSock.stUNIXAddr.sun_family = AF_UNIX;
    StrMov(oSvrSock.stUNIXAddr.sun_path, pszSockPath); // "/tmp/pipe_channel.sock"
    
    oSvrSock.pHandler = pHandler;   // Set callback handler
    
    if(dwConnTimeout) //如果设置了超时时间
    {
	int flags = 0;
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
	    m_sLastErrMsg = "[CEpollServerReactor::ConnectToUSockServer] connect to usock server ";
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
	    m_sLastErrMsg = "[CEpollServerReactor::ConnectToUSockServer] connect to usock server ";
	    m_sLastErrMsg += pszSockPath;
	    m_sLastErrMsg += " failed";

	    close(oSvrSock.iSockfd);
	    return -1;
	}

	if(!FD_ISSET(oSvrSock.iSockfd, &stWtFds))
	{
	    m_sLastErrMsg = "[CEpollServerReactor::ConnectToServer] connect to server ";
	    m_sLastErrMsg += pszSockPath;
	    m_sLastErrMsg += " timeout";

	    close(oSvrSock.iSockfd);
	    return -1;
	}	

	int error = 0;
	socklen_t len = sizeof(error);
	if(getsockopt(oSvrSock.iSockfd, SOL_SOCKET, SO_ERROR, &error, &len) < 0 || error)
	{
	    m_sLastErrMsg = "[CEpollServerReactor::ConnectToUSockServer] connect to usock server ";
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
        m_sLastErrMsg = "[CEpollServerReactor::ConnectToServer] fcntl F_GETFL failed";
        return -1;
    }
    
    if (fcntl(oSvrSock.iSockfd, F_SETFL, iCurrentFlag | FNDELAY) == -1)
    {
        close(oSvrSock.iSockfd);
        m_sLastErrMsg = "[CEpollServerReactor::ConnectToServer] fcntl F_SETFL failed";
        return -1;
    }
    
    m_arrTCPCliConnHandle[oSvrSock.iSockfd] = oSvrSock;

    // Set epoll flags
    FdAdd_i(oSvrSock.iSockfd, TCP_CLIENT_EVENT_FLAG, &(m_arrTCPCliConnHandle[oSvrSock.iSockfd]));
    
    // Callback
    pHandler->OnConnect(
        oSvrSock.iSockfd,  // ConnHandler
        0,
        0,
        0);
    
    return 0;
}

int CEpollServerReactor::RegisterUSockUDPServer(
                                              int& iServerHandle, // [OUT]
                                              const char* pszSockPath,
                                              bool bBind,
                                              bool bNoDelay,
											  bool bFDWatch)
{
    assert(pszSockPath != NULL);
    
    CUDPSockInfo oSvrSock;
    
    oSvrSock.iSockfd = ::socket(PF_LOCAL, SOCK_DGRAM, 0);
    if(oSvrSock.iSockfd < 0 )
    {
        m_sLastErrMsg = "[CEpollServerReactor::RegisterUSockUDPServer] create socket failed";
        return -1;
    }
    
    mode_t old_mod = umask(S_IRWXO); // Set umask = 002
    
    memset(&oSvrSock.stUNIXAddr, 0, sizeof(oSvrSock.stUNIXAddr));
    oSvrSock.stUNIXAddr.sun_family = AF_LOCAL;
    //StrMov(oSvrSock.stUNIXAddr.sun_path, pszSockPath); // "/tmp/pipe_channel.sock"
    strcpy(oSvrSock.stUNIXAddr.sun_path, pszSockPath);
    
    if(bBind)
    {
		const int on = 1;
		setsockopt(oSvrSock.iSockfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
        
        unlink(pszSockPath); // unlink it if exist

        if ((::bind(
            oSvrSock.iSockfd, 
            (struct sockaddr *)&(oSvrSock.stUNIXAddr),
            //sizeof(struct sockaddr))) == -1) 
            SUN_LEN(&(oSvrSock.stUNIXAddr)))) == -1)
        {
            close(oSvrSock.iSockfd);
			std::stringstream oss;
            oss << "[CEpollServerReactor::RegisterUSockUDPServer] bind socket failed";
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
            m_sLastErrMsg = "[CEpollServerReactor::RegisterUSockUDPServer] fcntl F_GETFL failed";
            return -1;
        }
        
        if (fcntl(oSvrSock.iSockfd, F_SETFL, iCurrentFlag | FNDELAY) == -1)
        {
            close(oSvrSock.iSockfd);
            m_sLastErrMsg = "[CEpollServerReactor::RegisterUSockUDPServer] fcntl F_SETFL failed";
            return -1;
        }
    }

    int flags = 0;
	flags = fcntl(oSvrSock.iSockfd, F_GETFL, 0);
    if (flags != -1)
    {
	    fcntl(oSvrSock.iSockfd, F_SETFL, flags | O_NONBLOCK);
    }
    
    m_oUDPSvrSockInfoMap[oSvrSock.iSockfd] = oSvrSock;
    iServerHandle = oSvrSock.iSockfd;

	if(bFDWatch)
	{
		FdAdd_i(oSvrSock.iSockfd, UDP_EVENT_FLAG);
	}
    
    return 0;
}

int CEpollServerReactor::RegisterUSockUDPPeerAddr(
                                                int& iPeerHandle,   // [OUT]
                                                const char* pszSockPath)
{
    if(!m_bNeedUDP)
    {
        m_sLastErrMsg = "[CEpollServerReactor::RegisterUSockUDPPeerAddr] m_bNeedUDP = false";
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

int CEpollServerReactor::USockUDPSendTo(
                                      int iServerHandle,
                                      const char* pSendBuf, 
                                      uint32_t nBufLen, 
                                      int iPeerHandle,
                                      int iFlag)
{
    if(!m_bNeedUDP)
    {
        m_sLastErrMsg = "[CEpollServerReactor::USockUDPSendTo] m_bNeedUDP = false";
        return -1;
    }
    
    // Check Server Handle
    hash_map<int, UDPSockInfo_T, hash<int> >::const_iterator ItSvr 
        = m_oUDPSvrSockInfoMap.find(iServerHandle);
    
    if(ItSvr == m_oUDPSvrSockInfoMap.end())
    {
        m_sLastErrMsg = "[CEpollServerReactor::USockUDPSendTo] unregisted sever handle. iServerHandle = ";
        std::ostringstream oss;
        oss << iServerHandle;
        m_sLastErrMsg += oss.str();
        
        return -1;
    }
    
    // Check Peer Handle
    hash_map<int, sockaddr_un, hash<int> >::const_iterator ItPeer
        = m_oUSockUDPPeerAddrInfoMap.find(iPeerHandle);
    
    if(ItPeer == m_oUSockUDPPeerAddrInfoMap.end())
    {
        m_sLastErrMsg = "[CEpollServerReactor::USockUDPSendTo] unregisted peer handle. iPeerHandle = ";
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
        m_sLastErrMsg = "[CEpollServerReactor::RegisterUSockUDPServer] sendto peer failed. errno = ";
        std::ostringstream oss1;
        oss1 << errno;
        m_sLastErrMsg += oss1.str();
        
        return -1;
    }
    
    return 0;
}

int CEpollServerReactor::USockUDPSendTo(
                                      int iServerHandle,
                                      const char* pSendBuf, 
                                      uint32_t nBufLen, 
                                      const char* pszSockPath,
                                      int iFlag)
{
    if(!m_bNeedUDP)
    {
        m_sLastErrMsg = "[CEpollServerReactor::USockUDPSendTo] m_bNeedUDP = false";
        return -1;
    }
    
    // Check Server Handle
    hash_map<int, UDPSockInfo_T, hash<int> >::const_iterator ItSvr 
        = m_oUDPSvrSockInfoMap.find(iServerHandle);
    
    if(ItSvr == m_oUDPSvrSockInfoMap.end())
    {
        m_sLastErrMsg = "[CEpollServerReactor::USockUDPSendTo] unregisted sever handle. iServerHandle = ";
        std::ostringstream oss;
        oss << iServerHandle;
        m_sLastErrMsg += oss.str();
        
        return -1;
    }
    
    // Make Peer Addr
    struct sockaddr_un stUNIXAddr;
    memset(&stUNIXAddr, 0, sizeof(stUNIXAddr));
    stUNIXAddr.sun_family = AF_LOCAL;
    StrMov(stUNIXAddr.sun_path, pszSockPath); // "/tmp/pipe_channel.sock"
    //strcpy(stUNIXAddr.sun_path, pszSockPath);
    
    // Send Buffer
    int iBytesSent = ::sendto(
        (*ItSvr).second.iSockfd,
        pSendBuf,
        nBufLen,
        iFlag,
        (struct sockaddr *)&(stUNIXAddr),
        SUN_LEN(&stUNIXAddr));
        //sizeof(struct sockaddr_un));
    
    if(iBytesSent == -1 || static_cast<uint32_t>(iBytesSent) != nBufLen)
    {
        m_sLastErrMsg = "[CEpollServerReactor::USockUDPSendTo] sendto peer failed. errno = ";
        std::ostringstream oss1;
        oss1 << errno << ", strerror:" << strerror(errno);
        m_sLastErrMsg += oss1.str();
        
        return -1;
    }
    
    return 0;
}

int CEpollServerReactor::USockUDPSendTo(
                                      const char* pSendBuf, 
                                      uint32_t nBufLen, 
                                      const char* pszSockPath,
                                      int iFlag)
{
    if(!m_bNeedUDP)
    {
        m_sLastErrMsg = "[CEpollServerReactor::USockUDPSendTo] m_bNeedUDP = false";
        return -1;
    }
    
    int iSockfd = ::socket(PF_LOCAL, SOCK_DGRAM, 0);
    if(iSockfd < 0 )
    {
        m_sLastErrMsg = "[CEpollServerReactor::USockUDPSendTo] create socket failed";
        return -1;
    }
    
    // Make Peer Addr
    struct sockaddr_un stUNIXAddr;
    memset(&stUNIXAddr, 0, sizeof(stUNIXAddr));
    stUNIXAddr.sun_family = AF_LOCAL;
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
        m_sLastErrMsg = "[CEpollServerReactor::USockUDPSendTo] sendto peer failed. errno = ";
        std::ostringstream oss1;
        oss1 << errno << ", strerror:" << strerror(errno);
        m_sLastErrMsg += oss1.str();
        
        return -1;
    }
    
    return 0;
}

char* CEpollServerReactor::StrMov(
                                register char *dst, 
                                register const char *src)
{
    while((*dst++ = *src++)) ;
    
    return dst-1;
}

////////////////////////////////////// Internal Functions ////////////////////////////////////
void CEpollServerReactor::FdAdd_i(
                                int iHandle, 
                                EventFlag_T enEventFlag, 
                                void* pvParam)
{
	if(-1 == m_epfd)
	{
		// Init epoll
		m_epfd = ::epoll_create(MAX_EPOLL_EVENT_NUM);
		if(m_epfd == -1)
		{
			if (errno == ENOSYS) // kernel doesn't support it
			{
				assert(0);
			}

			std::cerr << "epoll_create failed, errno = " << errno << ", " << strerror(errno) << std::endl;
			assert(0);
		}

		m_aEpollEvents = new epoll_event[MAX_EPOLL_EVENT_NUM];
		memset(m_aEpollEvents, 0, MAX_EPOLL_EVENT_NUM * sizeof(epoll_event));
	}

    switch(enEventFlag) 
    {
    case UDP_EVENT_FLAG:
        break;
    case TCP_CLIENT_EVENT_FLAG:
    case TCP_SERVER_EVENT_FLAG:
    case TCP_SERVER_ACCEPT_EVENT_FLAG:
        {
            assert(pvParam);
        }
        break;
    default:
        assert(0);
    }

	//fprintf(stderr,"pid=%d, FdAdd_i iHandle=%d,enEventFlag=0x%x,pvParam=%p\n ",getpid(),iHandle,enEventFlag,pvParam);

    m_oSelArray[iHandle].iHandle = iHandle;
    m_oSelArray[iHandle].enEventFlag = enEventFlag;
    m_oSelArray[iHandle].pvParam = pvParam;
    m_oSelArray[iHandle].stEpollEvent.events = EPOLLIN;
    
    m_oSelArray[iHandle].stEpollEvent.data.ptr = reinterpret_cast<void*>(&m_oSelArray[iHandle]);

    ++m_nEvents;

    // Add epoll event
    assert(m_nEvents > 0);

    if(::epoll_ctl(m_epfd, EPOLL_CTL_ADD, iHandle, &(m_oSelArray[iHandle].stEpollEvent)) != 0)
    {
        return;
    }
}

void CEpollServerReactor::FdDel_i(int iHandle)
{
    assert(iHandle == m_oSelArray[iHandle].iHandle);
    
    // Del epoll event
    if(::epoll_ctl(m_epfd, EPOLL_CTL_DEL, iHandle, &(m_oSelArray[iHandle].stEpollEvent)) != 0)
    {
        return;
    }
    
    m_oSelArray[iHandle].iHandle = 0;
    --m_nEvents;
}

void CEpollServerReactor::FdMod_i(int iHandle, bool bOnSendCheck)
{
    assert(iHandle == m_oSelArray[iHandle].iHandle);

    if(bOnSendCheck)
        m_oSelArray[iHandle].stEpollEvent.events = (EPOLLIN | EPOLLOUT);
    else
        m_oSelArray[iHandle].stEpollEvent.events = EPOLLIN;

    // Del epoll event
    if(::epoll_ctl(m_epfd, EPOLL_CTL_MOD, iHandle, &(m_oSelArray[iHandle].stEpollEvent)) != 0)
    {
        return;
    }
}

void CEpollServerReactor::ProcessTcpCliEvent_i(
                                             int iHandle, 
                                             uint32_t ulEvents,
                                             TCPSockInfo_T& rTcpCliSockInfo)
{
    ITCPNetHandler* l_pHandler = rTcpCliSockInfo.pHandler;
    assert(l_pHandler);

	if(IS_READ_AVAIL(ulEvents) 
			|| IS_ERR_HUP(ulEvents) // Error comes, Maybe read(...) == 0 next
	  )
    {
        m_iRecvBufLen = recv(
            iHandle, 
            m_achRecvBuf, 
            MAX_RECV_BUF_LEN, 
            0);
        
		if(m_iRecvBufLen < 0)
		{
			if(errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK)
			{
				return;
			}

			l_pHandler->OnClose(iHandle, 0, 0);
			this->Close(iHandle, false);
			return;
		}
        else if(m_iRecvBufLen == 0) // Reach Socket stream EOF
        {
			l_pHandler->OnClose(iHandle, 0, 0);
			this->Close(iHandle, false);
			return;
        }
        
        if(l_pHandler->OnRecv(
            iHandle,
            m_achRecvBuf,
            m_iRecvBufLen,
			0) < 0)
		{
			l_pHandler->OnClose(iHandle, 0, 0);
			this->Close(iHandle, false);
			return;
		}
    }
    
    if(IS_WRITE_AVAIL(ulEvents))
    {
        // Clear OnSend flag
        FdMod_i(iHandle, false);
        
        // Callback with OnSend(...)
        l_pHandler->OnSend(iHandle);
    }
}

void CEpollServerReactor::ProcessTcpSvrEvent_i(
                                             int iHandle, 
                                             uint32_t ulEvents,
                                             TCPSockInfo_T& rTcpSvrSockInfo)
{
    ITCPNetHandler* l_pHandler = rTcpSvrSockInfo.arrCloneTCPNetHandler[iHandle];
    assert(l_pHandler);

	if(IS_READ_AVAIL(ulEvents) 
			|| IS_ERR_HUP(ulEvents) // Error comes, Maybe read(...) == 0 next
	  )
    {
        m_iRecvBufLen = recv(
            iHandle, 
            m_achRecvBuf, 
            MAX_RECV_BUF_LEN, 
            MSG_NOSIGNAL);
        
        if(m_iRecvBufLen < 0)
		{
			if(errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK)
			{
				return;
			}
			l_pHandler->OnClose(iHandle, 0, 0);
			this->Close(iHandle, true);
			l_pHandler->Reset();
			//// Destroy cloned handler
			rTcpSvrSockInfo.arrCloneTCPNetHandler[iHandle] = NULL;
            return;
		}
        else if(m_iRecvBufLen == 0) // Reach Socket stream EOF
        {
			l_pHandler->OnClose(iHandle, 0, 0);
			this->Close(iHandle, true);
			l_pHandler->Reset();
			//// Destroy cloned handler
			rTcpSvrSockInfo.arrCloneTCPNetHandler[iHandle] = NULL;
            return;
        }
        
        assert(l_pHandler);
        
        if(l_pHandler->OnRecv(
            iHandle,
            m_achRecvBuf,
            m_iRecvBufLen,
            0) < 0)
		{
			l_pHandler->OnClose(iHandle, 0, 0);
			this->Close(iHandle, true);
			l_pHandler->Reset();
			//// Destroy cloned handler
			rTcpSvrSockInfo.arrCloneTCPNetHandler[iHandle] = NULL;
			return;
		}
    }
    
    if(IS_WRITE_AVAIL(ulEvents))
    {
        // Clear OnSend flag
        FdMod_i(iHandle, false);
        
        // Callback with OnSend(...)
        l_pHandler->OnSend(iHandle);
    }
}

void CEpollServerReactor::ProcessUDPEvent_i(int iHandle)
{
    static unsigned int l_uiHost = 0;
    static unsigned short l_ushPort = 0;

    m_iRecvBufLen = recvfrom(
        iHandle, 
        m_achRecvBuf, 
        MAX_RECV_BUF_LEN, 
        0, 
        (struct sockaddr*)&m_l_stFromAddr, 
        &m_l_iFromAddrLen);
    
    if(m_iRecvBufLen <= 0)
        return;
    
    l_uiHost = ntohl(m_l_stFromAddr.sin_addr.s_addr);
    l_ushPort = ntohs(m_l_stFromAddr.sin_port);
    
    // Callback
    if(m_pUDPNetHandler)
    {
        m_pUDPNetHandler->OnRecvFrom(
            iHandle, 
            m_achRecvBuf,
            m_iRecvBufLen,
            l_uiHost,
            l_ushPort,
            0);
    }
}

int CEpollServerReactor::CheckTCPComingConnection_i(TCPSockInfo_T& rTcpSvrSockInfo)
{
	int l_iSockFd = 0;
	unsigned int l_uiHost = 0;
	unsigned short l_ushPort = 0;
	ITCPNetHandler *l_pHandler = NULL;

    {
#ifdef __REACTOR_FUTEX_SWITCH__
        pthread_mutex_lock(m_pMutex);
#endif
    	l_iSockFd = accept(
    			rTcpSvrSockInfo.iSockfd, 
    			(struct sockaddr*)&m_l_stFromAddr, 
    			&m_l_iFromAddrLen);
#ifdef __REACTOR_FUTEX_SWITCH__
        pthread_mutex_unlock(m_pMutex);
#endif
    }
	if(l_iSockFd < 0)
	{
		if(errno != EAGAIN && errno != EWOULDBLOCK)
		{
			//ERROR_TRACE("TCP Server Accept failed");
		}
		return -1;
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
			return 0;
		}

		if(iPid == 0)   // new child process
		{
			// Do not accept new connection further,
			//// Only let main process do accept
			m_bNeedTCPServerAccept = false;   
			FdDel_i(rTcpSvrSockInfo.iSockfd);
		}
	}

	l_uiHost = m_l_stFromAddr.sin_addr.s_addr;
	l_ushPort = ntohs(m_l_stFromAddr.sin_port);

	m_arrTCPSvrConnHandle[l_iSockFd] = rTcpSvrSockInfo.iTCPSvrSockIndex;

	// Callback
	if(NULL == m_arrTCPNetHandler[l_iSockFd])
	{
		l_pHandler = rTcpSvrSockInfo.pHandler->Clone();
		assert(l_pHandler);
		m_arrTCPNetHandler[l_iSockFd] = l_pHandler;
	}
	else
	{
		l_pHandler = m_arrTCPNetHandler[l_iSockFd];
		l_pHandler->Reset();
	}
	rTcpSvrSockInfo.arrCloneTCPNetHandler[l_iSockFd] = l_pHandler;

	// Set Large RecvBuf for this socket
	//std::cout << "rTcpSvrSockInfo.ulRcvBufLen[" << rTcpSvrSockInfo.iRcvBufLen << "]" << std::endl;

	// Set NO DELAY
	linger m_sLinger;
	m_sLinger.l_onoff = 1;  // 在closesocket()调用,但是还有数据没发送完毕的时候容许逗留
	m_sLinger.l_linger = 0; // 容许逗留的时间为0秒
	setsockopt(l_iSockFd, SOL_SOCKET, SO_LINGER, (const char*)&m_sLinger, sizeof(linger));

	int iCurrentFlag = fcntl(l_iSockFd, F_GETFL, 0);
	fcntl(l_iSockFd, F_SETFL, iCurrentFlag | FNDELAY);

    //const int on = 1;
    //setsockopt(l_iSockFd, SOL_SOCKET, SO_KEEPALIVE, &on, sizeof(on));

	/*
	setsockopt(
			l_iSockFd, 
			SOL_SOCKET, 
			SO_RCVBUF, 
			reinterpret_cast<const void*>(&rTcpSvrSockInfo.iRcvBufLen),
			sizeof(rTcpSvrSockInfo.iRcvBufLen)
			);
*/
	// Set epoll flags
	FdAdd_i(l_iSockFd, TCP_SERVER_EVENT_FLAG, &rTcpSvrSockInfo);

	l_pHandler->OnConnect(
			l_iSockFd,  // ConnHandler
			l_uiHost,   // 网络字节序
			l_ushPort,
			0 );

	return 0;
}

uint32_t CEpollServerReactor::CleanTcpSvrHandle(int iBeginHandle, int iStep)
{
    int iEndHandle = iBeginHandle + iStep;
    if(iEndHandle >= (int)MAX_EPOLL_EVENT_NUM)
        iEndHandle = (int)MAX_EPOLL_EVENT_NUM - 1;
    
    uint32_t dwCnt = 0;
    ITCPNetHandler* l_pHandler = NULL;
    for(int i = iBeginHandle; i < iEndHandle; ++i)
    {
        l_pHandler = m_arrTCPNetHandler[i];
        if(l_pHandler && !(l_pHandler->IsConnected()))
        {
            delete l_pHandler;
            m_arrTCPNetHandler[i] = NULL;
            ++dwCnt;
        }
    }
    
    return dwCnt;
}

