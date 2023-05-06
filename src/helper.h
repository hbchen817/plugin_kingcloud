#pragma once

#include "message.h"
#include <stddef.h>
#include <openssl/aes.h>
#include <openssl/rand.h>
#include <openssl/evp.h>

size_t base64_encoded_len(size_t n);
size_t base64_encode(char *dst, const unsigned char *src, size_t len);
int fmt_RexProperty(char *buf, int len, const RexProperty_t *v);
int fmt_RexEvent(char *buf, int len, const RexEvent_t *v);
int fmt_RexService(char *buf, int len, const RexService_t *v);
int fmt_RexMessage(char *buf, int len, const RexMessage_t *v);

void get_time_str(char *buf);

/***********************************************************************
* 功能：将一个十六进制字节串转换成ASCII码表示的十六进制字符串
* 输入参数：pHex----十六进制数字节串首地址
*           pAscii---转换后ASCII码表示的十六进制字符串首地址
*           nLen---要转换的十六进制数长度（字节数）
* 转换后的结果全部是大写ASCII码表示的十六进制数
* 输入16字节16进制数 0x43,0x3b,0x0c,0x3d,0x0f,0xaa,0xbb,0xcc,0x11,0xdd,0x00,0x11,0x22,0x33,0x44,0x55 转换后为32字节字符串 433B0C3D0FAABBCC11DD001122334455
************************************************************************/
void encodeHex(unsigned char *pHex, unsigned char *pAscii, int nLen);

/***********************************************************************
* 功能：采用AES/ECB/PKCS5Padding方式进行解密
* 输入参数：ucInData----需要解密的数据
*           InDataLen---需要解密数据的字节长度
*           ucDecryptOutData---解密后的数据
*           ucDecryptKey---解密使用的密钥
* 填充长度方式为补齐16字节，填充数据为需要补齐的长度，如果需要填充5字节，则每字节填充数据为0x05; 数据长度满足16字节时需要填充16字节0x10
************************************************************************/

void AES_ECB_PKCS5Padding_decrypt(unsigned char *ucInData, int InDataLen, unsigned char *ucDecryptOutData, unsigned char *ucDecryptKey);

/***********************************************************************
* 功能：将一串16进制字符的字符串格式化 整形16进制数,存储在字节中，类似strtol
* 输入参数：str----十六进制字符串串首地址
*           out---转换后的16进制字节数组
*           outlen---转换后的字节长度
* 输入"02126397a1a89a95b2c788253e5063b0" 转换后为16字节16进制 0x02 12 63 97 a1 a8 9a 95 b2 c7 88 25 3e 50 63 b0
************************************************************************/

int decodeHex(char *str, unsigned char *out, unsigned int *outlen);

/***********************************************************************
* 功能：采用AES/ECB/PKCS5Padding方式进行加密
* 输入参数：ucInData----需要加密的数据
*           ucEncryptOutData---加密后的数据
*           OutDataLen---加密后数据字节长度
*           ucEncryptKey---加密使用的密钥
* 填充长度方式为补齐16字节，填充数据为需要补齐的长度，如果需要填充5字节，则每字节填充数据为0x05; 数据长度满足16字节时需要填充16字节0x10
************************************************************************/

void AES_ECB_PKCS5Padding_encrypt(unsigned char *ucInData, unsigned char *ucEncryptOutData, int *OutDataLen, unsigned char *ucEncryptKey);

unsigned char *hex_to_bytes(const char *hex_str);

char *bytes_to_hex(const unsigned char *bytes, int len);