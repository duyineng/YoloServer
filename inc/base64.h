// Base64.h
//

#ifndef _BASE64_H_
#define _BASE64_H_

#include <stdlib.h>
//#include <malloc.h>

//modify 2020-11-24
extern const char* Base64_Table;
//const char* Base64_Table = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

inline unsigned char CodeToIndex(char code)
{
	for(unsigned char k = 0; k < 64; k++)
	{
		if(code == Base64_Table[k])
		{
			return k;
		}
	}

	return 255;
}

inline int Base64Encode_len(int len)
{
	return ((len + 2) / 3 * 4) + 1;
}

inline char* Base64Encode(const unsigned char* src, long srclen, char* dest)
{
	long i, j;
	long len3b = (srclen / 3) * 3;
	long lenpading = srclen % 3;

	for(i = 0, j = 0; i < len3b; i += 3, j += 4)
	{
		dest[j] = Base64_Table[((int)src[i] & 0xFC) >> 2];
		dest[j+1] = Base64_Table[(((int)src[i] & 0x3) << 4) + (((int)src[i+1] & 0xF0) >> 4)];
		dest[j+2] = Base64_Table[(((int)src[i+1] & 0xF) << 2) + (((int)src[i+2] & 0xC0) >> 6)];
		dest[j+3] = Base64_Table[(int)src[i+2] & 0x3F];
	}
	if(lenpading > 0)
	{
		dest[j] = Base64_Table[((int)src[i] & 0xFC) >> 2];
		if(lenpading == 2)
		{
			dest[j+1] = Base64_Table[(((int)src[i] & 0x3) << 4) + (((int)src[i+1] & 0xF0) >> 4)];
			dest[j+2] = Base64_Table[((int)src[i+1] & 0xF) << 2];
		}
		else
		{
			dest[j+1] = Base64_Table[((int)src[i] & 0x3) << 4];
			dest[j+2] = '=';
		}
		dest[j+3] = '=';
		j += 4;
	}
	dest[j] = 0;

	return dest;
}

//inline int Base64Decode_len(const char *bufcoded)
//{
//	int nbytesdecoded;
//	register const unsigned char *bufin;
//	register int nprbytes;
//
//	bufin = (const unsigned char *) bufcoded;
//	while (pr2six[*(bufin++)] <= 63);
//
//	nprbytes = (bufin - (const unsigned char *) bufcoded) - 1;
//	nbytesdecoded = ((nprbytes + 3) / 4) * 3;
//
//	return nbytesdecoded + 1;
//}

inline unsigned char* Base64Decode(const char* src, long srclen, unsigned char* dest, long *pdestlen)
{
	long i, j;
	long len4b = (srclen / 4) * 4;
	unsigned char* pTemp = (unsigned char*)malloc(len4b);

	for(i = 0; i < len4b; i++){
		pTemp[i] = CodeToIndex(src[i]);
	}

	for(i = 0, j = 0; i < len4b; i += 4, j += 3){
		dest[j] = (unsigned char)((pTemp[i] << 2) + ((pTemp[i+1] & 0x30) >> 4));
		if(pTemp[i+2] >= 64){
			j += 1;
			break;
		}
		else if(pTemp[i+3] >= 64){
			dest[j+1] = (unsigned char)(((pTemp[i+1] & 0x0F) << 4) + ((pTemp[i+2] & 0x3C) >> 2));
			j += 2;
			break;
		}
		else{
			dest[j+1] = (unsigned char)(((pTemp[i+1] & 0x0F) << 4) + ((pTemp[i+2] & 0x3C) >> 2));
			dest[j+2] = (unsigned char)(((pTemp[i+2] & 0x03) << 6) + pTemp[i+3]);
		}
	}

	if(pdestlen != NULL)
		*pdestlen = j;

	free(pTemp);
	return dest;
}

int base64_encode(unsigned char *dest, int dest_len,
                  const unsigned char *src, int src_len);
int base64_decode(unsigned char *dest, int dest_len,
                  const unsigned char *src, int src_len);

#endif
