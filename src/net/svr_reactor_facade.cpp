
#include <assert.h>
#include "net_svr_reactor.h"
#include "svr_reactor_facade.h"

static CNetServerReactor s_oReactor;

////////////////////////
// class CReactor
CReactor::CReactor()
{
}

CReactor::~CReactor()
{
}

int CReactor::Initialize(
						 bool bNeedTimer,
						 bool bNeedUDP,
						 bool bNeedTCPServer,
						 bool bNeedTCPClient,
						 bool bNeedUserAction)
{
	return s_oReactor.Initialize(
		bNeedTimer, 
		bNeedUDP, 
		bNeedTCPServer, 
		bNeedTCPClient, 
		bNeedUserAction);
}

/////////////////// UDP /////////////////////
int CReactor::RegisterUDPServer(
								int& iServerHandle, // [OUT]
								unsigned short ushPort, 
								const char* pszHost,
								bool bNoDelay,
								bool bFDWatch)
{
	return s_oReactor.RegisterUDPServer(
		iServerHandle, ushPort, pszHost, bNoDelay, bFDWatch);
}

int CReactor::RegisterPeerAddr(
							   int& iPeerHandle,   // [OUT]
							   const char* pszHost,
							   unsigned short ushPort)
{
	return s_oReactor.RegisterPeerAddr(
		iPeerHandle,   // [OUT]
		pszHost,
		ushPort);
}

int CReactor::SendTo(
					 int iServerHandle,
					 const char* pSendBuf, 
					 uint32_t nBufLen, 
					 int iPeerHandle,
					 int iFlag)
{
	return s_oReactor.SendTo(
		iServerHandle,
		pSendBuf, 
		nBufLen, 
		iPeerHandle,
		iFlag);
}

int CReactor::SendTo(
					 int iServerHandle,
					 const char* pSendBuf, 
					 uint32_t nBufLen, 
					 const char* pszHost,
					 unsigned short ushPort,
					 int iFlag)
{
	return s_oReactor.SendTo(
		iServerHandle,
		pSendBuf, 
		nBufLen, 
		pszHost,
		ushPort,
		iFlag);
}

void CReactor::RegisterUDPNetHandler(IUDPNetHandler* pHandler)
{
	return s_oReactor.RegisterUDPNetHandler(
		pHandler);
}

/////////////////// TCP /////////////////////
int CReactor::RegisterTCPServer(
								int& iServerHandle, // [OUT]
								ITCPNetHandler* pHandler,
								unsigned short ushPort, 
								bool bParallel, // Multi-Process server
								int iRcvBufLen, // Default 128K
								const char* pszHost,
								bool bNoDelay,
								bool bFDWatch)
{
	return s_oReactor.RegisterTCPServer(
								iServerHandle, // [OUT]
								pHandler,
								ushPort, 
								bParallel, // Multi-Process server
								iRcvBufLen, // Default 128K
								pszHost,
								bNoDelay,
								bFDWatch);
}

int CReactor::ConnectToServer(
							  ITCPNetHandler* pHandler,
							  unsigned short ushServerPort, 
							  const char* pszServerHost,
							  uint32_t dwConnTimeout)
{
	return s_oReactor.ConnectToServer(
		pHandler,
		ushServerPort, 
		pszServerHost,
	        dwConnTimeout
		);
}

uint32_t CReactor::Send(
					  int iConnHandle,
					  char* pSendBuf, 
					  uint32_t nBufLen, 
					  int iFlag)
{
	return s_oReactor.Send(
		iConnHandle,
		pSendBuf, 
		nBufLen, 
		iFlag);
}

int CReactor::Close(
					int iConnHandle, 
					bool bAsServer)
{
	return s_oReactor.Close(
		iConnHandle, 
		bAsServer);
}

void CReactor::NeedOnSendCheck(int iConnHandle)
{
	return s_oReactor.NeedOnSendCheck(iConnHandle);
}

/////////////////// Unix TCP Socket /////////////
int CReactor::RegisterUSockTCPServer(
									 int& iServerHandle, // [OUT]
									 ITCPNetHandler* pHandler,
									 const char* pszSockPath,
									 bool bParallel, // Multi-Process server
									 int iRcvBufLen, // Default 128K
									 bool bNoDelay,
									 mode_t mask,
									 bool bFDWatch)
{
	return s_oReactor.RegisterUSockTCPServer(
		iServerHandle, // [OUT]
		pHandler,
		pszSockPath,
		bParallel, // Multi-Process server
		iRcvBufLen, // Default 128K
		bNoDelay,
		mask,
		bFDWatch);
}

int CReactor::ConnectToUSockTCPServer(
									  ITCPNetHandler* pHandler,
									  const char* pszSockPath,
									  uint32_t dwConnTimeout
									  )
{
	return s_oReactor.ConnectToUSockTCPServer(
		pHandler,
		pszSockPath,
		dwConnTimeout
		);
}

/////////////////// Unix UDP Socket /////////////
int CReactor::RegisterUSockUDPServer(
									 int& iServerHandle, // [OUT]
									 const char* pszSockPath,
									 bool bBind,
									 bool bNoDelay,
									 bool bFDWatch)
{
	return s_oReactor.RegisterUSockUDPServer(
		iServerHandle, // [OUT]
		pszSockPath,
		bBind,
		bNoDelay,
		bFDWatch);
}

int CReactor::RegisterUSockUDPPeerAddr(
									   int& iPeerHandle,   // [OUT]
									   const char* pszSockPath)
{
	return s_oReactor.RegisterUSockUDPPeerAddr(
		iPeerHandle,   // [OUT]
		pszSockPath);
}

int CReactor::USockUDPSendTo(
							 int iServerHandle,
							 const char* pSendBuf, 
							 uint32_t nBufLen, 
							 const char* pszSockPath,
							 int iFlag)
{
	return s_oReactor.USockUDPSendTo(
							 iServerHandle,
							 pSendBuf, 
							 nBufLen, 
							 pszSockPath,
							 iFlag);
}

int CReactor::USockUDPSendTo(
							 const char* pSendBuf, 
							 uint32_t nBufLen, 
							 const char* pszSockPath,
							 int iFlag)
{
	return s_oReactor.USockUDPSendTo(
							 pSendBuf, 
							 nBufLen, 
							 pszSockPath,
							 iFlag);
}

/////////////////// Timer ///////////////////
int CReactor::RegisterTimer(
							int& iTimerID,              // [OUT]
							ITimerHandler* pHandler,
							const CTimeValue& tvInterval,
							unsigned int dwCount       // Count for timeout, 0 means endless loop   
							)
{
	return s_oReactor.RegisterTimer(
		iTimerID,              // [OUT]
		pHandler,
		tvInterval,
		dwCount       // Count for timeout, 0 means endless loop   
		);
}

int CReactor::UnregisterTimer(int iTimerID)
{
	return s_oReactor.UnregisterTimer(iTimerID);
}

/////////////////// User Event ///////////////////
void CReactor::RegisterUserEventHandler(
										IUserEventHandler* pUserEventHandler
										)
{
	return s_oReactor.RegisterUserEventHandler(
		pUserEventHandler
		);
}

/////////////////// Event Loop ///////////////////
void CReactor::RunEventLoop()
{
	return s_oReactor.RunEventLoop();
}

void CReactor::StopEventLoop()
{
	return s_oReactor.StopEventLoop();
}

/////////////////// Error Msg Description ///////////////////
const char* CReactor::GetLastErrMsg() const
{
	return s_oReactor.GetLastErrMsg();
}

int CReactor::CheckEvents()
{
	return s_oReactor.CheckEvents();
}

int CReactor::ProcessSocketEvent()
{
	return s_oReactor.ProcessSocketEvent();
}

uint32_t CReactor::CleanTcpSvrHandle(int iBeginHandle, int iStep)
{
	return s_oReactor.CleanTcpSvrHandle(iBeginHandle, iStep);
}

void CReactor::FDWatchServer(
		int iTcpServerHandle, 
		int iUdpServerHandle, 
		int iUSockTcpServerHandle, 
		int iUSockUdpServerHandle)
{
	return s_oReactor.FDWatchServer(
			iTcpServerHandle, iUdpServerHandle, 
			iUSockTcpServerHandle, iUSockUdpServerHandle);
}


