#ifndef NET_SVR_REACTOR_H
#define NET_SVR_REACTOR_H

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/un.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <map>
#include <vector>
#include <string>
#include "reactor_handler.h"
#include "timer_heap_queue.h"

const uint32_t MAX_RECV_BUF_LEN = 1024 * 1024;

class CNetServerReactor
{
	friend class CReactor;
public:
	CNetServerReactor();
	~CNetServerReactor();

public:

	int Initialize(bool bNeedTimer = true, bool bNeedUDP = true,
			bool bNeedTCPServer = true, bool bNeedTCPClient = true,
			bool bNeedUserAction = true);

	void FDWatchServer(int iTcpServerHandle, int iUdpServerHandle,
			int iUSockTcpServerHandle, int iUSockUdpServerHandle);

	/////////////////// UDP /////////////////////
	int RegisterUDPServer(int& iServerHandle, // [OUT]
			unsigned short ushPort, const char* pszHost = NULL, bool bNoDelay =
					true, bool bFDWatch = true);

	int RegisterPeerAddr(int& iPeerHandle,   // [OUT]
			const char* pszHost, unsigned short ushPort);

	int SendTo(int iServerHandle, const char* pSendBuf, uint32_t nBufLen,
			int iPeerHandle, int iFlag = 0);

	int SendTo(int iServerHandle, const char* pSendBuf, uint32_t nBufLen,
			const char* pszHost, unsigned short ushPort, int iFlag = 0);

	void RegisterUDPNetHandler(IUDPNetHandler* pHandler);

	/////////////////// TCP /////////////////////
	int RegisterTCPServer(int& iServerHandle, // [OUT]
			ITCPNetHandler* pHandler, unsigned short ushPort, bool bParallel =
					false, // Multi-Process server
			int iRcvBufLen = 131072, // Default 128K
			const char* pszHost = NULL, bool bNoDelay = true, bool bFDWatch =
					true);

	int ConnectToServer(ITCPNetHandler* pHandler, unsigned short ushServerPort,
			const char* pszServerHost, uint32_t dwConnTimeout = 0);

	int Send(int iConnHandle, char* pSendBuf, uint32_t nBufLen, int iFlag = 0);

	int Close(int iConnHandle, bool bAsServer);

	void NeedOnSendCheck(int iConnHandle);

	/////////////////// Unix TCP Socket /////////////
	int RegisterUSockTCPServer(int& iServerHandle, // [OUT]
			ITCPNetHandler* pHandler, const char* pszSockPath, bool bParallel =
					false, // Multi-Process server
			int iRcvBufLen = 131072, // Default 128K
			bool bNoDelay = true, mode_t mask = S_IRWXO, // Default 002
			bool bFDWatch = true);

	int ConnectToUSockTCPServer(ITCPNetHandler* pHandler,
			const char* pszSockPath, uint32_t dwConnTimeout = 0);

	/////////////////// Unix UDP Socket /////////////
	int RegisterUSockUDPServer(
			int& iServerHandle, // [OUT]
			const char* pszSockPath, bool bBind = true, bool bNoDelay = true,
			bool bFDWatch = true);

	int RegisterUSockUDPPeerAddr(int& iPeerHandle,   // [OUT]
			const char* pszSockPath);

	int USockUDPSendTo(int iServerHandle, const char* pSendBuf,
			uint32_t nBufLen, int iPeerHandle, int iFlag = 0);

	int USockUDPSendTo(int iServerHandle, const char* pSendBuf,
			uint32_t nBufLen, const char* pszSockPath, int iFlag = 0);

	int USockUDPSendTo(const char* pSendBuf, uint32_t nBufLen,
			const char* pszSockPath, int iFlag = 0);

	/////////////////// Timer ///////////////////
	int RegisterTimer(
			int& iTimerID,              // [OUT]
			ITimerHandler* pHandler, const CTimeValue& tvInterval,
			unsigned int dwCount = 0 // Count for timeout, 0 means endless loop
			);

	int UnregisterTimer(int iTimerID);

	/////////////////// User Event ///////////////////
	void RegisterUserEventHandler(IUserEventHandler* pUserEventHandler);

	/////////////////// Event Loop ///////////////////
	void RunEventLoop();
	void StopEventLoop();
	uint32_t CleanTcpSvrHandle(int iBeginHandle, int iStep);

	/////////////////// Error Msg Description ///////////////////
	const char* GetLastErrMsg() const;

protected:
	int CheckEvents();
	int ProcessSocketEvent();

	int CheckTCPComingConnection_i();

	char* StrMov(register char *dst, register const char *src);

private:

	//////////////////// UDP ///////////////////////
	typedef struct CUDPServerSockInfo
	{
		CUDPServerSockInfo() :
				iSockfd(0)
		{
			memset(&stINETAddr, 0, sizeof(struct sockaddr_in));
			memset(&stUNIXAddr, 0, sizeof(struct sockaddr_un));
		}

		struct sockaddr_in stINETAddr;    // AF_INET addr
		struct sockaddr_un stUNIXAddr;   // AF_UNIX addr
		int iSockfd;
	} UDPServerSockInfo_T;

	// Index with 'iServerHandle'
	std::map<int, CUDPServerSockInfo> m_oUDPSvrSockInfoMap;
	int m_iCurrentUDPSvrSockIndex;
	// Index with 'iPeerHandle'
	std::map<int, sockaddr_in> m_oUDPPeerAddrInfoMap;
	std::map<int, sockaddr_un> m_oUSockUDPPeerAddrInfoMap;
	int m_iCurrentUDPPeerAddrIndex;

	IUDPNetHandler* m_pUDPNetHandler;

	//////////////////// TCP ///////////////////////
	typedef struct CTCPServerSockInfo
	{
		CTCPServerSockInfo() :
				iSockfd(0), pHandler(NULL), bParallel(false)
		{
			memset(&stINETAddr, 0, sizeof(struct sockaddr_in));
			memset(&stUNIXAddr, 0, sizeof(struct sockaddr_un));
		}

		struct sockaddr_in stINETAddr;    // AF_INET addr
		struct sockaddr_un stUNIXAddr;   // AF_UNIX addr
		int iSockfd;
		int iRcvBufLen;
		ITCPNetHandler* pHandler;   // Cloneable
		// <ConnHandle, *>
		std::map<int, ITCPNetHandler*> oCloneTCPNetHandlerMap;
		bool bParallel;
	} TCPServerSockInfo_T;

	// For TCP Server
	// Index with 'iServerHandle'
	std::map<int, CTCPServerSockInfo> m_oTCPSvrSockInfoMap;
	// <ConnHandle, ServerHandle>
	std::map<int, int> m_oTCPSvrConnHandleMap;
	int m_iCurrentTCPSvrSockIndex;

	// For TCP Client
	// Index with 'ConnHandle'
	std::map<int, CTCPServerSockInfo> m_oTCPClientConnHandleMap;

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
	int m_iNumOfFds;
	fd_set m_setReadFds;
	fd_set m_setWriteFds;
	fd_set m_setReadFdsHolder;
	fd_set m_setWriteFdsHolder;

	char m_achRecvBuf[MAX_RECV_BUF_LEN];
	int m_iRecvBufLen;

	bool m_bNeedTimer;
	bool m_bNeedUDP;
	bool m_bNeedTCPServer;
	bool m_bNeedTCPServerAccept;
	bool m_bNeedTCPClient;
	bool m_bNeedUserAction;

private:
	/*
	 *	For temporary using
	 */
	int m_l_iSockfd;
	struct sockaddr_in m_l_stFromAddr;
	socklen_t m_l_iFromAddrLen;
	int m_l_iPeerHandle;
	std::map<int, TCPServerSockInfo_T>::iterator m_l_tcp_svr_it;
	std::map<int, TCPServerSockInfo_T>::iterator m_l_tcp_cli_it;
	std::map<int, UDPServerSockInfo_T>::iterator m_l_udp_svr_it;
	std::map<int, int>::iterator m_l_conn_it;
};

#endif /* NET_SVR_REACTOR_H */

