#include "aes.h"
#include "stdio.h"
#include "string.h"

void log(char *head,unsigned char *data,unsigned char len)
{
    unsigned char i;

    printf("%s:",head);

    for(i=0;i<len;i++)
    {
        printf("%02X ",data[i]);
    }
    printf("\r\n");
}

int main(int argc, char *argv[])
{
    //秘钥
    unsigned char key[16]={0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39,0x30,0x41,0x42,0x43,0x44,0x45,0x46};
    //明文 10Bytes
    unsigned char source[10]={0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39,0x30};

    unsigned char temp[16]={0};
    unsigned char ret;

    printf("AES-128 pkcs7 ECB test\r\n\r\n");

    log("KEY   ",key,16);
    AES_Init(key);

    memcpy(temp,source,10);
    log("source",key,10);

    ret=AES_add_pkcs7Padding(temp, 10);
    log("padd  ",temp,ret);

    AES_Encrypt(temp, temp,ret, NULL);
    log("encode",temp,ret);

    AES_Decrypt(temp, temp,ret, NULL);
    log("decode",temp,ret);

    ret=AES_delete_pkcs7Padding(temp, ret);
    log("source",temp,ret);

    return 0;
}
