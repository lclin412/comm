
#ifndef EPOLL_SVR_REACTOR_H
#define EPOLL_SVR_REACTOR_H

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/un.h>
#include <sys/epoll.h>
#include <sys/types.h>
#include <sys/stat.h>

#if __GNUC__ > 3
#include <ext/hash_map>
using namespace __gnu_cxx;
#else
#include <hash_map>
using namespace std;
#endif

#include <string>
#include "reactor_handler.h"
#include "timer_heap_queue.h"

#ifdef __REACTOR_FUTEX_SWITCH__
#include <pthread.h>
#include <sys/mman.h>
#endif

#ifdef __IM_SERVER_CONFIG__
	//config for IM server
	const uint32_t MAX_RECV_BUF_LEN = 10 * 1024;
	const uint32_t MAX_EPOLL_EVENT_NUM = 65535;
	const uint32_t MAX_SERVER_ITEM_NUM = 32;
	const uint32_t MAX_AS_CLIENT_NUM = 32;
#else
	const uint32_t MAX_RECV_BUF_LEN = 1024 * 1024;
	const uint32_t MAX_EPOLL_EVENT_NUM = 4096;
	const uint32_t MAX_SERVER_ITEM_NUM = 128;
	const uint32_t MAX_AS_CLIENT_NUM = 128;
#endif


class CEpollServerReactor
{
	friend class CReactor;
public:
    CEpollServerReactor();
    ~CEpollServerReactor();
    
public:
    
    int Initialize(
        bool bNeedTimer = true,
        bool bNeedUDP = true,
        bool bNeedTCPServer = true,
        bool bNeedTCPClient = true,
        bool bNeedUserAction = true);
    
    /////////////////// UDP /////////////////////
    int RegisterUDPServer(
        int& iServerHandle, // [OUT]
        unsigned short ushPort, 
        const char* pszHost = NULL,
        bool bNoDelay = true,
		bool bFDWatch = true);
    
    int RegisterPeerAddr(
        int& iPeerHandle,   // [OUT]
        const char* pszHost,
        unsigned short ushPort);
    
    int SendTo(
        int iServerHandle,
        const char* pSendBuf, 
        uint32_t nBufLen, 
        int iPeerHandle,
        int iFlag = 0);
    
    int SendTo(
        int iServerHandle,
        const char* pSendBuf, 
        uint32_t nBufLen, 
        const char* pszHost,
        unsigned short ushPort,
        int iFlag = 0);
    
    void RegisterUDPNetHandler(
        IUDPNetHandler* pHandler);

    /////////////////// TCP /////////////////////
    int RegisterTCPServer(
        int& iServerHandle, // [OUT]
        ITCPNetHandler* pHandler,
        unsigned short ushPort, 
        bool bParallel = false, // Multi-Process server
        int iRcvBufLen = 131072, // Default 128K
        const char* pszHost = NULL,
        bool bNoDelay = true,
		bool bFDWatch = true);

    int ConnectToServer(
        ITCPNetHandler* pHandler,
        unsigned short ushServerPort, 
        const char* pszServerHost,
	uint32_t dwConnTimeout = 0
        );

    int Send(
        int iConnHandle,
        char* pSendBuf, 
        uint32_t nBufLen, 
        int iFlag = 0);

    int Close(
        int iConnHandle, 
        bool bAsServer);
    
    void NeedOnSendCheck(int iConnHandle);

	void FDWatchServer(
			int iTcpServerHandle, 
			int iUdpServerHandle, 
			int iUSockTcpServerHandle, 
			int iUSockUdpServerHandle);
    
    /////////////////// Unix TCP Socket /////////////
    int RegisterUSockTCPServer(
        int& iServerHandle, // [OUT]
        ITCPNetHandler* pHandler,
        const char* pszSockPath,
        bool bParallel = false, // Multi-Process server
        int iRcvBufLen = 131072, // Default 128K
        bool bNoDelay = true,
		mode_t mask = S_IRWXO, // Default 002
		bool bFDWatch = true
		);
    
    int ConnectToUSockTCPServer(
        ITCPNetHandler* pHandler,
        const char* pszSockPath,
	uint32_t dwConnTimeout
        );

    /////////////////// Unix UDP Socket /////////////
    int RegisterUSockUDPServer(
        int& iServerHandle, // [OUT]
        const char* pszSockPath,
        bool bBind = true,
        bool bNoDelay = true,
		bool bFDWatch = true
		);

    int RegisterUSockUDPPeerAddr(
        int& iPeerHandle,   // [OUT]
        const char* pszSockPath);

    int USockUDPSendTo(
        int iServerHandle,
        const char* pSendBuf, 
        uint32_t nBufLen, 
        int iPeerHandle,
        int iFlag = 0);
    
    int USockUDPSendTo(
        int iServerHandle,
        const char* pSendBuf, 
        uint32_t nBufLen, 
        const char* pszSockPath,
        int iFlag = 0);

    int USockUDPSendTo(
        const char* pSendBuf, 
        uint32_t nBufLen, 
        const char* pszSockPath,
        int iFlag = 0);

    /////////////////// Timer ///////////////////
    int RegisterTimer(
        int& iTimerID,              // [OUT]
        ITimerHandler* pHandler,
        const CTimeValue& tvInterval,
        unsigned int dwCount = 0       // Count for timeout, 0 means endless loop   
        );
    
    int UnregisterTimer(int iTimerID);
    
    /////////////////// User Event ///////////////////
    void RegisterUserEventHandler(
        IUserEventHandler* pUserEventHandler
        );

    /////////////////// Event Loop ///////////////////
    void RunEventLoop();
    void StopEventLoop();
    uint32_t CleanTcpSvrHandle(int iBeginHandle, int iStep);
    
    /////////////////// Error Msg Description ///////////////////
    const char* GetLastErrMsg() const;
 
	ITCPNetHandler* GetTcpHandlerByFd(uint32_t fd)
	{
		if(fd >= MAX_EPOLL_EVENT_NUM)
			return NULL;

		return m_arrTCPNetHandler[fd];
	}

private:
    int CheckEvents();
    int ProcessSocketEvent();
    
    char* StrMov(
        register char *dst, 
        register const char *src);
    
private:
    
    //////////////////// EPOLL ///////////////////////
    int m_epfd;
    epoll_event* m_aEpollEvents;

    typedef enum
    {
        NONE_FLAG = 0,
        TCP_CLIENT_EVENT_FLAG = 1,
        TCP_SERVER_EVENT_FLAG,
        TCP_SERVER_ACCEPT_EVENT_FLAG,
        UDP_EVENT_FLAG,
    } EventFlag_T;
    
    struct SelItem_T
    {
        SelItem_T() : iHandle(0), enEventFlag(NONE_FLAG), pvParam(NULL)
        { memset(&stEpollEvent, 0 , sizeof(epoll_event)); }

        void Reset()
        {
            iHandle = 0;
            enEventFlag = NONE_FLAG;
            pvParam = NULL;

            memset(&stEpollEvent, 0 , sizeof(epoll_event));
        }
        
        int  iHandle;
        EventFlag_T enEventFlag;
        void* pvParam;

        epoll_event stEpollEvent;
    };

    SelItem_T m_oSelArray[MAX_EPOLL_EVENT_NUM];
    int m_iEcc;
    uint32_t m_nEvents;

    void FdAdd_i(int iHandle, EventFlag_T enEventFlag, void* pvParam = NULL);
    void FdDel_i(int iHandle);
    void FdMod_i(int iHandle, bool bOnSendCheck);

    //////////////////// UDP ///////////////////////
    typedef struct CUDPSockInfo
    {
        CUDPSockInfo()
            : iSockfd(0)
        {
            memset(&stINETAddr, 0, sizeof(struct sockaddr_in));
            memset(&stUNIXAddr, 0, sizeof(struct sockaddr_un));
        }
        
        struct sockaddr_in stINETAddr;    // AF_INET addr
        struct sockaddr_un stUNIXAddr;   // AF_UNIX addr
        int iSockfd;
    } UDPSockInfo_T;
    
    // Index with 'iServerHandle'
    hash_map<int, UDPSockInfo_T, hash<int> > m_oUDPSvrSockInfoMap;
    int m_iCurrentUDPSvrSockIndex;
    // Index with 'iPeerHandle'
    hash_map<int, sockaddr_in, hash<int> > m_oUDPPeerAddrInfoMap;
    hash_map<int, sockaddr_un, hash<int> > m_oUSockUDPPeerAddrInfoMap;
    int m_iCurrentUDPPeerAddrIndex;
    
    IUDPNetHandler* m_pUDPNetHandler;

    //////////////////// TCP ///////////////////////
    typedef struct CTCPSockInfo
    {
        CTCPSockInfo()
            : iSockfd(0)
            , iTCPSvrSockIndex(0)
            , pHandler(NULL)
            , bParallel(false)
        {
            memset(&stINETAddr, 0, sizeof(struct sockaddr_in));
            memset(&stUNIXAddr, 0, sizeof(struct sockaddr_un));
			memset(&arrCloneTCPNetHandler, 0, sizeof(arrCloneTCPNetHandler));
        }
        
        struct sockaddr_in stINETAddr;    // AF_INET addr
        struct sockaddr_un stUNIXAddr;   // AF_UNIX addr
        int iSockfd;
        int iTCPSvrSockIndex;
        int iRcvBufLen;
        ITCPNetHandler* pHandler;   // Cloneable

		ITCPNetHandler* arrCloneTCPNetHandler[MAX_EPOLL_EVENT_NUM];
        bool bParallel;
    } TCPSockInfo_T;

    // For TCP Server
	//TCPSockInfo_T m_arrTCPSvrSockInfo[MAX_EPOLL_EVENT_NUM];
	TCPSockInfo_T m_arrTCPSvrSockInfo[MAX_SERVER_ITEM_NUM];
	int m_arrTCPSvrConnHandle[MAX_EPOLL_EVENT_NUM];
    int m_iCurrentTCPSvrSockIndex;

    // For TCP Client
	TCPSockInfo_T m_arrTCPCliConnHandle[MAX_AS_CLIENT_NUM];
    
	ITCPNetHandler* m_arrTCPNetHandler[MAX_EPOLL_EVENT_NUM];
    //////////////////// Timer /////////////////////
    CTimerHeapQueue m_oTimerQueue;
    int m_iCurrentTimerIndex;
    CTimeValue m_l_tvRemain;

    //////////////////// User Event ////////////////
    IUserEventHandler* m_pUserEventHandler;

    //////////////////// Common ////////////////////
    std::string m_sLastErrMsg;
    bool m_bEventStop;
    struct timeval m_stTimeVal;
   
    char m_achRecvBuf[MAX_RECV_BUF_LEN];
    int m_iRecvBufLen;

    bool m_bNeedTimer;
    bool m_bNeedUDP;
    bool m_bNeedTCPServer;
    bool m_bNeedTCPServerAccept;
    bool m_bNeedTCPClient;
    bool m_bNeedUserAction;

private:
    int CheckTCPComingConnection_i(TCPSockInfo_T& rTcpSvrSockInfo);
    
    void ProcessTcpCliEvent_i(
        int iHandle, 
        uint32_t ulEvents,
        TCPSockInfo_T& rTcpCliSockInfo);
    
    void ProcessTcpSvrEvent_i(
        int iHandle, 
        uint32_t ulEvents,
        TCPSockInfo_T& rTcpSvrSockInfo);
    
    void ProcessUDPEvent_i(int iHandle);
    
private:
    /*
    *	For temporary using
    */
    int m_l_iSockfd;
    struct sockaddr_in m_l_stFromAddr;
    socklen_t m_l_iFromAddrLen;
    int m_l_iPeerHandle;
    hash_map<int, UDPSockInfo_T, hash<int> >::iterator m_l_udp_svr_it;
    
#ifdef __REACTOR_FUTEX_SWITCH__
    pthread_mutex_t *m_pMutex;
#endif
};

#endif /* EPOLL_SVR_REACTOR_H */

