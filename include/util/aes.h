#ifndef _AES_20140317_H_
#define _AES_20140317_H_

#define Bits128				16
#define Bits192				24
#define Bits256				32
#define ENCRYPT_BLOCK_SIZE	16
#define SUCESS 0
#define TRUE 1

#include "md5.h"
#include <stdio.h>
#include <malloc.h>

typedef unsigned char _u8;
typedef int _int32;
typedef unsigned int _u32;

typedef struct
{
	_int32 Nb;
	_int32 Nk;
	_int32 Nr;
	_u8 State[4][4];
	_u8 key[32];
	_u8 w[16 * 15];
} ctx_aes;

enum AESKeyLength
{
	AES_KEY_LENGTH_16 = 16, AES_KEY_LENGTH_24 = 24, AES_KEY_LENGTH_32 = 32
};

namespace comm
{
namespace util
{
class AES
{
public:
	AES();
	~AES()
	{
		if (Sbox != NULL)
		{
			delete []Sbox;
			Sbox = NULL;
		}

		if (iSbox != NULL)
		{
			delete []iSbox;
			iSbox = NULL;
		}

		if (Rcon != NULL)
		{
			delete []Rcon;
			Rcon = NULL;
		}
	}
public:
	int decrypt4aes(const std::string &inData, const std::string &strKey,
			std::string &outData, std::string &errMsg);
	int encrypt4aes(const std::string &inData, const std::string &strKey,
			std::string &outData, std::string &errMsg);
private:
	void aes_init(ctx_aes* aes, int keySize, _u8* keyBytes);
	void aes_cipher(ctx_aes* aes, _u8* input, _u8* output);
	void aes_invcipher(ctx_aes* aes, _u8* input, _u8* output);
	void SetNbNkNr(ctx_aes* aes, _int32 keyS);
	void AddRoundKey(ctx_aes* aes, _int32 round);
	void SubBytes(ctx_aes* aes);
	void InvSubBytes(ctx_aes* aes);
	void ShiftRows(ctx_aes* aes);
	void InvShiftRows(ctx_aes* aes);
	void MixColumns(ctx_aes* aes);
	void InvMixColumns(ctx_aes* aes);
	_u8 gfmultby01(_u8 b);
	_u8 gfmultby02(_u8 b);
	_u8 gfmultby03(_u8 b);
	unsigned char gfmultby09(unsigned char b);
	unsigned char gfmultby0b(unsigned char b);
	unsigned char gfmultby0d(unsigned char b);
	unsigned char gfmultby0e(unsigned char b);
	void KeyExpansion(ctx_aes* aes);
	void SubWord(_u8 *word, _u8 *result);
	void RotWord(_u8 *word, _u8 *result);
	_int32 aes_encrypt_with_known_key(char* buffer, _u32* len, _u8 *key,std::string &outData);
	_int32 aes_decrypt_with_known_key(char* p_data_buff, _u32* p_data_buff_len,
			_u8 *key,std::string &outData);
private:
	_u8 *Sbox;
	_u8 *iSbox;
	_u8 *Rcon;
};
}
} //comm::util
#endif//_AES_20140317_H_

