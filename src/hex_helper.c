#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// 将十六进制字符转换成对应的数值，例如 'a' 转换为 10
static unsigned char hex2int(char ch)
{
    if (ch >= '0' && ch <= '9') {
        return ch - '0';
    } else if (ch >= 'a' && ch <= 'f') {
        return ch - 'a' + 10;
    } else if (ch >= 'A' && ch <= 'F') {
        return ch - 'A' + 10;
    } else {
        return 0;
    }
}

// 将十六进制字符串解码为二进制数据
// 返回解码后的数据长度
size_t mqtt_decodeHex(const char *hexstr, unsigned char *buf, size_t bufsize)
{
    size_t length = strlen(hexstr) / 2;
    if (length > bufsize) {
        length = bufsize;
    }
    memset(buf, 0, length);
    for (size_t i = 0; i < length; i++) {
        buf[i] = (hex2int(hexstr[i * 2]) << 4) | hex2int(hexstr[i * 2 + 1]);
    }
    return length;
}

// 将二进制数据编码为十六进制字符串
// 返回编码后的字符串长度
size_t mqtt_encodeHex(const unsigned char *buf, size_t buflen, char *hexstr, size_t hexsize)
{
    size_t length = buflen * 2;
    if (length + 1 > hexsize) {
        length = hexsize - 1;
    }
    for (size_t i = 0; i < length / 2; i++) {
        sprintf(&hexstr[i * 2], "%02X", buf[i]);
    }
    hexstr[length] = '\0';
    return length;
}
