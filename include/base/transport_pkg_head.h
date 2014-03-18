#ifndef __TRANSPORT_PKG_HEAD_H__
#define __TRANSPORT_PKG_HEAD_H__

#include <stdint.h>

#define  SOOF          1179602771

typedef enum
{
  CLIENT2SERVER_REQ = 115,   //�ͻ��������������Ϣ
  SERVER2CLIENT_RES = 511,   //��������Ӧ�ͻ�����Ϣ
  SERVER2CLIENT_REQ = 116,   //����������ͻ�����Ϣ
  CLIENT2SERVER_RES = 611,   //�ͻ�����Ӧ�������Ϣ
} MSG_TYPE;

#pragma pack(push, 1)

typedef struct tagTRANSPORT_PKG_HEAD
{
  uint32_t magic_quote;       //SOOF���ַ���
  uint16_t version;           //�汾
  uint16_t msg_type;          //��Ϣ����
  uint16_t service_id;        //����id
  uint8_t  call_level;        //������õ����
  uint64_t session_id;        //�Ựid
  uint32_t route_id;          //·��id
  uint32_t reserve;           //����
  uint32_t length;            //Э���峤��
  uint32_t squenceid;         //squenceid
} TRANSPORT_PKG_HEAD, *PTRANSPORT_PKG_HEAD;

#pragma pack(pop)

#endif

