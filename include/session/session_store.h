

#ifndef __SESSION_STORE_H__
#define __SESSION_STORE_H__


#include <iostream>
#include <string>
#include <vector>
#include <map>

#include <libmemcached-1.0/memcached.h>

using namespace std;

enum session_return_t{session_success, session_notfound, session_error};

class CSession_store
{
public:
    CSession_store();
    ~CSession_store();
    
    session_return_t init(int);
    session_return_t addServer(string const &, unsigned short);
    session_return_t addServer(map<string, unsigned short> const &);
    
    session_return_t set(string const &, string const &);
    session_return_t sets(string const &, string const &, uint64_t const); 

    session_return_t get(string const &, string &);
    session_return_t gets(string const &, string &, uint64_t &);   
    
    session_return_t del(string const &key);

/*
    void set(time_t timeout_data, uint32_t flags, int timeout_ms, time_t time_reconn);
    
    string GetLastErrMsg() const;
    
    bool init(const string& strip, unsigned short port);
    bool init(const map<string, unsigned short>& mapsrv);

    bool setBehavior(memcached_behavior_t flag, uint64_t data);
    
    // return: 0.exist 
    //         1.not exist 
    //        -1.other error, maybe memcached die and call GetLastErrMsg get errmsg
    int gets(const string& strkey, string& strval);
    bool sets(const string& strkey, const string& strval);

protected:
    int md_connect(const string& strip, unsigned short port);
    void new_memcached(memcached_st *clone);

private:
    bool error(std::string& error_message) const;
    bool error() const;
    int get_error_server(string& strip, unsigned short *port);
    int set_get_failed();        
*/

private:
    memcached_st               *m_memdb;
//    time_t                      m_expiration;
//    uint32_t                    m_flags;
//    string                      m_errmsg;

private:
    int                         m_timeout;
//    map<string, unsigned short> m_mapSrv;

//    time_t                      m_reConnTime;
//    time_t                      m_tmConnect;
//    map<string, unsigned short> m_mapHostDead;

};


#endif



