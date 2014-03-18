#ifndef _CRYPTO_UTIL_H_
#define _CRYPTO_UTIL_H_

#define CRYPTOPP_ENABLE_NAMESPACE_WEAK 1

#include <iostream>
#include <sstream>
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <string>

#include "aes.h"
#include "md5.h"
#include "hex.h"
#include "files.h"
#include "default.h"
#include "filters.h"
#include "osrng.h"

using namespace CryptoPP;

enum AESKeyLength
{
	AES_KEY_LENGTH_16 = 16, AES_KEY_LENGTH_24 = 24, AES_KEY_LENGTH_32 = 32
};

class CCryptoUtil
{
public:
	static int encrypt4aes(const std::string &inData, const std::string &strKey,
			std::string &outData, std::string &errMsg)
	{
		outData = "";
		errMsg = "";

		if (inData.empty() || strKey.empty())
		{
			errMsg = "indata or key is empty!!";
			return -1;
		}

		unsigned int iKeyLen = strKey.length();

		if (iKeyLen != AES_KEY_LENGTH_16 && iKeyLen != AES_KEY_LENGTH_24
				&& iKeyLen != AES_KEY_LENGTH_32)
		{
			errMsg = "aes key invalid!!";
			return -2;
		}

		byte iv[AES::BLOCKSIZE];
		int iResult = 0;

		try
		{
			CBC_Mode<AES>::Encryption e;
			e.SetKeyWithIV((byte*) strKey.c_str(), iKeyLen, iv);
			StringSource ss(inData, true,
					new StreamTransformationFilter(e, new StringSink(outData)));
		} catch (const CryptoPP::Exception& e)
		{
			errMsg = "Encryptor throw exception!!";
			iResult = -3;
		}

		return iResult;
	}

	static int decrypt4aes(const std::string &inData, const std::string &strKey,
			std::string &outData, std::string &errMsg)
	{
		outData = "";
		errMsg = "";

		if (inData.empty() || strKey.empty())
		{
			errMsg = "indata or key is empty!!";
			return -1;
		}

		unsigned int iKeyLen = strKey.length();

		if (iKeyLen != AES_KEY_LENGTH_16 && iKeyLen != AES_KEY_LENGTH_24
				&& iKeyLen != AES_KEY_LENGTH_32)
		{
			errMsg = "aes key invalid!!";
			return -2;
		}

		byte iv[AES::BLOCKSIZE];
		int iResult = 0;

		try
		{
			CBC_Mode<AES>::Decryption d;
			d.SetKeyWithIV((byte*) strKey.c_str(), iKeyLen, iv);
			StringSource ss(inData, true,
					new StreamTransformationFilter(d, new StringSink(outData)));
		}
		catch (const CryptoPP::Exception& e)
		{
			errMsg = "Encryptor throw exception";
			iResult = -3;
		}

		return iResult;
	}

	static std::string md5(const std::string& inData)
	{
		std::string digest;
		Weak1::MD5 md5;
		StringSource(inData, true,
				new HashFilter(md5, new HexEncoder(new StringSink(digest))));

		return digest;
	}
};

int main(int argc, char**argv)
{
	std::string strKeyReed = "key";
	std::string strValue = "vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv";
	std::string strKey = CCryptoUtil::md5(strKeyReed);

	std::cout << strKey << std::endl;

	std::string strResult;
	std::string strErrMsg;

	std::cout << strValue << std::endl;
	CCryptoUtil::encrypt4aes(strValue, strKey, strResult, strErrMsg);
	CCryptoUtil::decrypt4aes(strResult, strKey, strValue, strErrMsg);

	std::cout << strValue << std::endl;
}

#endif//_CRYPTO_UTIL_H_
