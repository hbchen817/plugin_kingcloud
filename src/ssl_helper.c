
#include <openssl/aes.h>
#include "helper.h"
#include <string.h>

/***********************************************************************
* 功能：将一个十六进制字节串转换成ASCII码表示的十六进制字符串
* 输入参数：pHex----十六进制数字节串首地址
*           pAscii---转换后ASCII码表示的十六进制字符串首地址
*           nLen---要转换的十六进制数长度（字节数）
* 转换后的结果全部是大写ASCII码表示的十六进制数
* 输入16字节16进制数 0x43,0x3b,0x0c,0x3d,0x0f,0xaa,0xbb,0xcc,0x11,0xdd,0x00,0x11,0x22,0x33,0x44,0x55 转换后为32字节字符串 433B0C3D0FAABBCC11DD001122334455
************************************************************************/
void encodeHex(unsigned char *pHex, unsigned char *pAscii, int nLen)
{
    unsigned char Nibble[2];
    unsigned int i,j;
    for (i = 0; i < nLen; i++){
        Nibble[0] = (pHex[i] & 0xF0) >> 4;
        Nibble[1] = pHex[i] & 0x0F;
        for (j = 0; j < 2; j++){
            if (Nibble[j] < 10){
                Nibble[j] += 0x30;
            }
            else{
                if (Nibble[j] < 16)
                    Nibble[j] = Nibble[j] - 10 + 'A';
            }
            *pAscii++ = Nibble[j];
        }
    }
}

/***********************************************************************
* 功能：采用AES/ECB/PKCS5Padding方式进行解密
* 输入参数：ucInData----需要解密的数据
*           InDataLen---需要解密数据的字节长度
*           ucDecryptOutData---解密后的数据
*           ucDecryptKey---解密使用的密钥
* 填充长度方式为补齐16字节，填充数据为需要补齐的长度，如果需要填充5字节，则每字节填充数据为0x05; 数据长度满足16字节时需要填充16字节0x10
************************************************************************/

void AES_ECB_PKCS5Padding_decrypt(unsigned char *ucInData, int InDataLen, unsigned char *ucDecryptOutData, unsigned char *ucDecryptKey)
{
	int i;
	AES_KEY aesDecryptKey;
	int Padding;

	AES_set_decrypt_key(ucDecryptKey, 128, &aesDecryptKey);

	for(i = 0;i < InDataLen/16;i++)
	{
		AES_ecb_encrypt(ucInData+i*16, ucDecryptOutData+i*16, &aesDecryptKey, AES_DECRYPT);
	}

	Padding = ucDecryptOutData[InDataLen-1];
	if(Padding > 0 && Padding <= 16)
	{
		
		for(i = 0; i < Padding; i++)
		{
			ucDecryptOutData[InDataLen-Padding+i] = '\0';
		}
	}
}

/***********************************************************************
* 功能：将一串16进制字符的字符串格式化 整形16进制数,存储在字节中，类似strtol
* 输入参数：str----十六进制字符串串首地址
*           out---转换后的16进制字节数组
*           outlen---转换后的字节长度
* 输入"02126397a1a89a95b2c788253e5063b0" 转换后为16字节16进制 0x02 12 63 97 a1 a8 9a 95 b2 c7 88 25 3e 50 63 b0
************************************************************************/

int decodeHex(char *str, unsigned char *out, unsigned int *outlen)
{
    char *p = str;
    char high = 0, low = 0;
    int tmplen = 0, cnt = 0;
    tmplen = strlen(p);
    while(cnt < (tmplen / 2))
    {
        high = ((*p > '9') && ((*p <= 'F') || (*p <= 'f'))) ? *p - 48 - 7 : *p - 48;
        low = (*(++ p) > '9' && ((*p <= 'F') || (*p <= 'f'))) ? *(p) - 48 - 7 : *(p) - 48;
        out[cnt] = ((high & 0x0f) << 4 | (low & 0x0f));
        p ++;
        cnt ++;
    }
    if(tmplen % 2 != 0) out[cnt] = ((*p > '9') && ((*p <= 'F') || (*p <= 'f'))) ? *p - 48 - 7 : *p - 48;
    
    if(outlen != NULL) *outlen = tmplen / 2 + tmplen % 2;
    return tmplen / 2 + tmplen % 2;
}


/***********************************************************************
* 功能：采用AES/ECB/PKCS5Padding方式进行加密
* 输入参数：ucInData----需要加密的数据
*           ucEncryptOutData---加密后的数据
*           OutDataLen---加密后数据字节长度
*           ucEncryptKey---加密使用的密钥
* 填充长度方式为补齐16字节，填充数据为需要补齐的长度，如果需要填充5字节，则每字节填充数据为0x05; 数据长度满足16字节时需要填充16字节0x10
************************************************************************/

void AES_ECB_PKCS5Padding_encrypt(unsigned char *ucInData, unsigned char *ucEncryptOutData, int *OutDataLen, unsigned char *ucEncryptKey)
{
    int i;
    int k;
    int Datalen;
    int PaddingLen;
    int totlelen;
    AES_KEY aesEncryptKey;

    Datalen = strlen((char *)ucInData);
    PaddingLen = (Datalen%16);

    if(PaddingLen == 0)
    {
        for(k=0; k<16; k++)     
        ucInData[Datalen+k] = 0x10; 
        totlelen = Datalen+16;
    }
    else
    {
        for(k=0;k<(16-PaddingLen);k++) 
            ucInData[Datalen+k]=(16-PaddingLen); 
        totlelen = Datalen+16-PaddingLen;
    }
    AES_set_encrypt_key(ucEncryptKey, 128, &aesEncryptKey);

    for(i = 0;i < totlelen/16;i++)
    {
        AES_ecb_encrypt(ucInData+i*16, ucEncryptOutData+i*16, &aesEncryptKey, AES_ENCRYPT);
    }

    *OutDataLen = totlelen;
}

/* 将十六进制字符串转换为字节数组 */
unsigned char *hex_to_bytes(const char *hex_str)
{
    int len = strlen(hex_str) / 2;
    unsigned char *result = (unsigned char *) malloc(len);

    for (int i = 0; i < len; i++) {
        sscanf(hex_str + 2 * i, "%2hhx", result + i);
    }

    return result;
}

/* 将字节数组转换为十六进制字符串 */
char *bytes_to_hex(const unsigned char *bytes, int len)
{
    char *result = (char *) malloc(len * 2 + 1);

    for (int i = 0; i < len; i++) {
        sprintf(result + 2 * i, "%02x", bytes[i]);
    }

    return result;
}