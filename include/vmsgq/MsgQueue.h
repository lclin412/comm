#ifndef _MY_MSG_QUEUE_
#define _MY_MSG_QUEUE_

#include <stdio.h>  
#include <stdlib.h>  
#include <fcntl.h>  
#include <string.h>  
#include <unistd.h>  
#include <errno.h>
#include <sys/types.h>  
#include <sys/ipc.h>  
#include <sys/msg.h>  
#include <string>

class MsgQueue
{
public:
    MsgQueue():m_msgId(-1){}
    ~MsgQueue(){}

    #define MAX_LENGTH 1024*50

    struct _msg_struct_ 
    {
        long type;
        char msg[MAX_LENGTH];  //max buffer is 50k
    };

public:
    int InitQueue(unsigned int key)
    {
        if((m_msgId = msgget((key_t)key,0666|IPC_CREAT)) < 0 ) 
        {
            m_errmsg = strerror(errno);
            return -1;
        }

        return 0;
    }

    int PutOne(const std::string str, long type = 9999)
    {
        if(m_msgId == -1)
        {
            m_errmsg = "Not Init";
            return -1;
        }

        if(str.length() > MAX_LENGTH)
        {
            m_errmsg = "Msg Too large";
            return -2;
        }

        struct _msg_struct_ s_data;
        s_data.type = type;
        memcpy(s_data.msg, str.c_str(), str.length());

        int ret = msgsnd(m_msgId, (void*)&s_data, str.length(), IPC_NOWAIT);
        if(ret < 0)
        {
            m_errmsg = strerror(errno);
            return -3;
        }

        return 0;
    }

    int GetOne(std::string &strOut, long type = 0)
    {

        if(m_msgId == -1)
        {
            m_errmsg = "Not Init";
            return -1;
        }

        struct _msg_struct_ s_data; 

        ssize_t size = msgrcv(m_msgId, (void*)&s_data, MAX_LENGTH, type, IPC_NOWAIT);
        if(size < 0)
        {
            m_errmsg = strerror(errno);
            return -2;
        }

        strOut.assign(s_data.msg, size);

        return size;
    }

    
    int DelQueue()
    {
        if(msgctl(m_msgId, IPC_RMID, 0) < 0)
        {
            m_errmsg = strerror(errno);
            return -1;
        }

        return 0;
    }

    std::string GetLastErrMsg()
    {
        return m_errmsg;
    }

private:
    int m_msgId;
    std::string m_errmsg;
};

#endif

