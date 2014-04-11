#ifndef _20140317_MD5_H_
#define _20140317_MD5_H_

#include <string>
#include <memory.h>
#include <ctype.h>
#include <sstream>
#include <iostream>
#include <stdio.h>

namespace comm
{
namespace util
{

typedef struct _tag_ctx_md5
{
	unsigned int _state[4];
	unsigned int _count[2];
	unsigned char _inner_data[64];
} ctx_md5, *p_ctx_md5;

class MD5
{
public:
	MD5(){}
	~MD5(){}
private:
	void md5_initialize(ctx_md5* p_ctx);
	void md5_update(ctx_md5* p_ctx, const unsigned char *pdata,
			unsigned int count);
	void md5_finish(ctx_md5* p_ctx, unsigned char cid[16]);
	void md5_handle(unsigned int *state, const unsigned char block[64]);
	void md5_encode(unsigned char *output, const unsigned int *input,
			unsigned int len);
	void md5_decode(unsigned int *output, const unsigned char *input,
			unsigned int len);
public:
	int md5(const char* cInput,int iLength,char* cOut);
};
}
} //comm::util
#endif//_20140317_MD5_H_
