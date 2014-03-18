
#ifndef REACTOR_HANDLER_H
#define REACTOR_HANDLER_H

#include <stdint.h>

class IUDPNetHandler
{
public:
    virtual ~IUDPNetHandler() {};
    
public:
    virtual int OnRecvFrom(
        int iServerHandle,
        char* pRecvBuf,
        uint32_t nBufLen,
        unsigned int uiPeerHost,
        unsigned short ushPeerPort,
        int iFlag) = 0;
};

class ITCPNetHandler
{
public:
    virtual ~ITCPNetHandler() {};

    virtual ITCPNetHandler* Clone() const = 0;
    virtual void Reset() = 0;
    
public:
    virtual int OnConnect(
        int iConnHandle,
        uint32_t uiPeerHost,
        uint16_t ushPeerPort,
        int iFlag) = 0;

    virtual int OnRecv(
        int iConnHandle,
        char* pRecvBuf,
        uint32_t nBufLen,
        int iFlag) = 0;

	virtual int OnSend(
		int iConnHandle
		) = 0;

    virtual int OnClose(
        int iConnHandle,
        int iCloseCode,
        int iFlag) = 0;

    virtual int SendData(
        const char* pSendBuf = NULL,
        uint32_t nBufLen = 0
        ) = 0;

    virtual int Close() = 0;

	virtual bool IsConnected() const = 0;
};

class ITimerHandler
{
public:
    virtual ~ITimerHandler() {};
    
public:
    virtual int OnTimer(
        int iTimerID) = 0;
};

class IUserEventHandler
{
public:
    virtual ~IUserEventHandler() {};
    
public:
    /*
     *	Return 0 means Can fire Event
     */
    virtual int CheckEvent(
        void* pvParam = NULL) = 0;

    virtual int OnEventFire(
        void* pvParam = NULL) = 0;
};

#endif /* REACTOR_HANDLER_H */

