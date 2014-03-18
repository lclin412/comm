
                    
#ifndef SVR_REACTOR_FACADE_H
#define SVR_REACTOR_FACADE_H

#include "time_value.h"
#include "reactor_handler.h"
#include <sys/types.h>
#include <sys/stat.h>

class CReactor
{
public:
    CReactor();
    ~CReactor();
    
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

    uint32_t Send(
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
	uint32_t dwConnTimeout = 0
        );

    /////////////////// Unix UDP Socket /////////////
    int RegisterUSockUDPServer(
        int& iServerHandle, // [OUT]
        const char* pszSockPath,
        bool bBind = true,
        bool bNoDelay = true,
		bool bFDWatch = true);

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
    
    ITCPNetHandler* GetTcpHandlerByFd(uint32_t fd);

protected:
	int CheckEvents();
	int ProcessSocketEvent();
};

#endif /* SVR_REACTOR_FACADE_H */

