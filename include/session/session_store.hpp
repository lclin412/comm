

#ifndef __SESSION_STORE_H__
#define __SESSION_STORE_H__


#include <iostream>
#include <string>
#include <vector>
#include <map>

#include <libmemcached-1.0/memcached.hpp>


using namespace std;
using namespace memcache;


class CSession_store: public Memcache
{

public:
    CSession_store()
    {
         expiration = 0;
         flags = 0;
    }

    // init add server, and call error() judge success or failed.
    CSession_store(const string& strhost, unsigned short port)
    : Memcache(strhost, port)
    {
         expiration = 0;
         flags = 0;
    }

    ~CSession_store()
    {
    }
  
    string GetLastErrMsg()
    {
        m_errmsg = "";
        error(m_errmsg);
        return m_errmsg;
    }

    // connect another one memcached server
    // if failed call error(string&) or GetLastErrMsg get errmsg
    bool init(const string& strip, unsigned short port)
    {
         if (!addServer(strip, port))
         {
             return false;
         }
         return true;
    }

    // connect memcached cache serverlist
    // if failed call error(string&) or GetLastErrMsg get errmsg
    bool init(map<string, unsigned short>& mapsvr)
    {
        map<string, unsigned short>::const_iterator it;
        it = mapsvr.begin();
        while (it != mapsvr.end())
        {
            if (!addServer(it->first, it->second))
                return false;
            ++it;
        }
        return true;
    }

    void set_time_out(time_t tm)
    {
        expiration = tm;
    }

    void set_flags(uint32_t flag)
    {
        flags = flag;
    }


    // set string value
    bool sets(const string& strkey, const string& strvalue)
    {
        return set(strkey, strvalue.c_str(), strvalue.size(), expiration, flags);
    }

    // get string value
    bool gets(const string& strkey, string& strval)
    {
        vector<char> ret_val;
        if (get(strkey, ret_val))
        {
           strval = &ret_val[0];
           strval[ret_val.size()] = 0;
           return true;
        }
        return false;       
    }

    // get multi values 
    // return key,value map
    bool get_multi(std::vector<std::string>& keys, std::map<string, string>& values)
    {		
        if (!mget(keys))
        {
            return false;
        } 

        string key;
        string value;
        std::vector<char> ret_val;
        vector<string>::const_iterator it = keys.begin();
        while (it != keys.end())
        {
            if (memcached_success(fetch((key = *it), ret_val)))
            {
                value = &ret_val[0];
                value[ret_val.size()] = 0;
                values[*it] = value;
            }
            ++it;
        }
        return true;
    }

    bool del(const string& strkey)
    {
        return remove(strkey);
    }

    bool clear()
    {
        return flush(expiration);
    }

private:
    time_t        expiration;
    uint32_t      flags;

    string        m_errmsg;
};






#endif


