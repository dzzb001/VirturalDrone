#pragma once
#include <string>

#ifndef _WIN32
typedef unsigned char byte;
#endif

namespace Rtsp
{

class CEncrypt
{
public:
	CEncrypt(void);
	~CEncrypt(void);

	void MD5Data(const byte * pSrcData, unsigned int dwSrcLen, char * pszMD5);

	std::string base64Encode(char const* origSigned, unsigned origLength);
	unsigned char* base64Decode(char const* buff, unsigned int & resultSize, bool trimTrailingZeros = true);

private:

	/* MD5 context. */
	typedef struct MD5Context 
	{
		unsigned int state[4];		/* state (ABCD) */
		unsigned int count[2];		/* number of bits, modulo 2^64 (lsb first) */
		byte buffer[64];	/* input buffer */
	} MD5_CTX;

	void Encode(byte *output, unsigned int * input, unsigned int len);
	void Decode(unsigned int * output, const byte *input, unsigned int len);
	void MD5Transform(unsigned int state[4], const byte block[64]);

	void Init(MD5_CTX *);
	void Update(MD5_CTX *, const byte *, unsigned int);
	void Final(byte [16], MD5_CTX *);
	void End(MD5_CTX *, char *);
	static void initBase64DecodeTable();
	char* strDupSize(char const* str);
};

}