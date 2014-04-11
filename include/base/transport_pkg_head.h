#ifndef __TRANSPORT_PKG_HEAD_H__
#define __TRANSPORT_PKG_HEAD_H__

#include <stdint.h>

#define  SOOF     1397706566

typedef enum
{
  CLIENT2SERVER_REQ = 115,   //客户端请求服务器消息
  SERVER2CLIENT_RES = 511,   //服务器响应客户端消息
  SERVER2CLIENT_REQ = 116,   //服务器请求客户端消息
  CLIENT2SERVER_RES = 611,   //客户端响应服务端消息
} MSG_TYPE;

typedef enum
{
  SERVICE_MSG = 10000,       //msg域业务
  SERVICE_MY  = 10001,		   //my域业务
  SERVICE_IM  = 10002,		   //im域业务
  SERVICE_RPC = 10003,       //RPC业务
} SERVICE_CODE;

#pragma pack(push, 1)

typedef struct tagTRANSPORT_PKG_HEAD
{
  uint32_t magic_quote;       //SOOF的字符串
  uint16_t version;           //版本
  uint16_t msg_type;          //消息类型
  uint16_t service_id;        //服务id
  uint8_t  call_level;        //服务调用的深度
  uint64_t session_id;        //会话id
  uint32_t route_id;          //路由id
  uint32_t reserve;           //保留
  uint32_t squenceid;         //squenceid
  uint64_t ignore_session_id; //忽略会话id
  uint8_t  reserve_head[21];  //保留头
  uint32_t length;            //协议体长度
} TRANSPORT_PKG_HEAD, *PTRANSPORT_PKG_HEAD;

#pragma pack(pop)

#endif