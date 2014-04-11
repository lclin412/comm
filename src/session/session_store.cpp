

#include <sys/epoll.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <errno.h>

#include <glog/logging.h>

#include <session_store.h>


CSession_store::CSession_store():m_memdb(NULL), m_timeout(0)
{
    /*m_timeout = 0;
    m_flags = 0;
    m_tmConnect = 3000;
    m_reConnTime= 10;

    m_md = memcached_create(NULL);

    pthread_t pt;*/
    
}

CSession_store::~CSession_store()
{
   if (m_memdb) memcached_free(m_memdb); 
}


session_return_t CSession_store::init(int time)
{
    m_timeout = time;
    if (!m_memdb) m_memdb = memcached_create(NULL);
    if (!m_memdb) return session_error; 
   

    memcached_return_t rt = memcached_behavior_set(m_memdb, 
                            MEMCACHED_BEHAVIOR_SUPPORT_CAS, true);
    
    if (MEMCACHED_SUCCESS != rt)
        return session_error;
    
    return session_success;
}


session_return_t CSession_store::addServer(string const &ip, unsigned short port)
{
    if (!m_memdb)
    {
        LOG(ERROR)<<"memcached not init"<<endl;
        return session_error;
    }

    memcached_return_t rt = memcached_server_add(m_memdb, ip.c_str(), port);
    
    if (MEMCACHED_SUCCESS != rt)
        return session_error;
    
    return session_success;
}

session_return_t CSession_store::addServer(map<string, unsigned short> const &servers)
{
    if (!m_memdb)
    {
        LOG(ERROR)<<"memcached not init"<<endl;
        return session_error;
    }
    
    memcached_return_t rt;
    map<string, unsigned short>::const_iterator iter = servers.begin();
    for (; iter != servers.end(); ++iter)
    {
        rt = memcached_server_add(m_memdb, iter->first.c_str(), iter->second);
        if (MEMCACHED_SUCCESS != rt) return session_error;
    }
    
    return session_success;
}

session_return_t CSession_store::set(string const &key, string const &value)
{
    if (!m_memdb)
    {
        LOG(ERROR)<<"memcached not init"<<endl;
        return session_error;
    }
    
    memcached_return_t rt;
    rt = memcached_set(m_memdb, key.c_str(), key.length(), value.c_str(), 
                      value.length(), m_timeout, 0);
    
    //we not set buffer_requests, so return 
    //MEMCACHED_SUCCESS or error, not MEMCACHED_BUFFERED
    if (MEMCACHED_SUCCESS != rt)
        return session_error;
    
    return session_success;    
}

session_return_t CSession_store::sets(string const &key, string const &value, uint64_t const cas)
{
    if (!m_memdb)
    {
        LOG(ERROR)<<"memcached not init"<<endl;
        return session_error;
    }

    memcached_return_t rt;
    rt = memcached_cas(m_memdb, key.c_str(), key.length(), value.c_str(),
                      value.length(), m_timeout, 0, cas);

    //we not set buffer_requests, so return 
    //MEMCACHED_SUCCESS or error, not MEMCACHED_BUFFERED
    if (MEMCACHED_SUCCESS != rt)
        return session_error;

    return session_success;
}

session_return_t CSession_store::get(string const &key, string &value)
{
    value.clear();    

    if (!m_memdb)
    {
        LOG(ERROR)<<"memcached not init"<<endl;
        return session_error;
    }
    
    char const * chkey = key.c_str();
    size_t klen = key.length();
    
    memcached_return_t rt;
    rt = memcached_mget(m_memdb, &chkey, &klen, 1);
    if (MEMCACHED_SUCCESS != rt) 
    {
        LOG(ERROR)<<"memcached get failed"<<endl;
        return session_error;
    }

    memcached_result_st  result;
    memcached_result_st *presult = memcached_result_create(m_memdb, &result);
    presult = memcached_fetch_result(m_memdb, presult, &rt);   
    
    if (MEMCACHED_NOTFOUND == rt) return session_notfound;
    if (MEMCACHED_SUCCESS != rt) return session_error;
    
    char const *chvalue = memcached_result_value(presult);
    if (!chvalue)
    {
        LOG(ERROR)<<"memcached get value is null"<<endl;
        
        memcached_result_free(presult);
        return session_error;  
    }
    
    size_t length = memcached_result_length(presult);
    value.assign(chvalue, length);

    memcached_result_free(presult);
    return session_success;
}

session_return_t CSession_store::gets(string const &key, string &value, uint64_t &cas)
{
    value.clear();
    
    if (!m_memdb)
    {
        LOG(ERROR)<<"memcached not init"<<endl;
        return session_error;
    }
    
    /*const char *keys [2] = {key.c_str(), NULL};
    size_t klen[2] = {key.length(), 0};
    */
     
    char const *chkey = key.c_str();
    size_t klen = key.length();

    memcached_return_t rt;
    rt = memcached_mget(m_memdb, &chkey, &klen, 1);
    if (MEMCACHED_SUCCESS != rt) 
    {
        LOG(ERROR)<<"memcached mget failed"<<endl;
        return session_error;
    }

    memcached_result_st  result;
    memcached_result_st *presult = memcached_result_create(m_memdb, &result);
    presult = memcached_fetch_result(m_memdb, presult, &rt);
    
/*    if (!presult)
    {
        if (MEMCACHED_NOTFOUND == rt */ /*|| MEMCACHED_END == rt)*/ 
/*        {
            LOG(ERROR)<<"memcached key not exsit"<<endl;
            return true;
        }
        
        LOG(ERROR)<<"memcached fetch result failed"<<endl;
        return false;
        
    }
 
    if (MEMCACHED_SUCCESS != rt)
    {
        memcached_result_free(presult);
        
        LOG(ERROR)<<"memcached fetch result failed"<<endl;
        return false;
    }*/

   
    if (MEMCACHED_NOTFOUND == rt)  return session_notfound;
    if (MEMCACHED_SUCCESS != rt) return session_error;
   
    cas = memcached_result_cas(presult);
    if (!cas)
    {
        memcached_result_free(presult);
        return session_error;
    }
    
    char const * chvalue = memcached_result_value(presult);
    if (!chvalue)
    {
        memcached_result_free(presult);
        return session_error;
    }

    size_t length = memcached_result_length(presult);
    value.assign(chvalue, length);
    
    memcached_result_free(presult);
    return session_success;
}

session_return_t CSession_store::del(string const &key)
{
    if (!m_memdb)
    {
        LOG(ERROR)<<"memcached not init"<<endl;
        return session_error;
    }

    memcached_return_t rt;
    rt = memcached_delete(m_memdb, key.c_str(), key.length(), 0);
    
    //we not set buffer_requests, so return 
    //MEMCACHED_SUCCESS or error, not MEMCACHED_BUFFERED 
    if (MEMCACHED_NOTFOUND == rt) return session_notfound;   
    
    if (MEMCACHED_SUCCESS != rt) 
        return session_error;

    return session_success;    
}

