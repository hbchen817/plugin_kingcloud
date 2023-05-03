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

// 生成随机字节数组作为IV
void gen_random_iv(unsigned char *iv, int iv_len);
// 将十六进制字符串转换为字节数组
void hex_to_bytes(const char *hex_str, unsigned char *byte_arr, int hex_len);
// 将字节数组转换为十六进制字符串
void bytes_to_hex(const unsigned char *byte_arr, int byte_len, char *hex_str);
// 使用AES-128-CBC加密
void aes_encrypt(const unsigned char *key, const unsigned char *iv, 
                    const unsigned char *input, int input_len, unsigned char *output);
// 使用AES-128-CBC解密
void aes_decrypt(const unsigned char *key, const unsigned char *iv, 
                    const unsigned char *input, int input_len, unsigned char *output);
