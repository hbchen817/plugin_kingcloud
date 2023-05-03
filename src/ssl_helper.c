#include "helper.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// 生成随机字节数组作为IV
void gen_random_iv(unsigned char *iv, int iv_len) {
    RAND_bytes(iv, iv_len);
}

// 将十六进制字符串转换为字节数组
void hex_to_bytes(const char *hex_str, unsigned char *byte_arr, int hex_len) {
    int i;
    for (i = 0; i < hex_len / 2; i++) {
        sscanf(hex_str + i * 2, "%2hhx", &byte_arr[i]);
    }
}

// 将字节数组转换为十六进制字符串
void bytes_to_hex(const unsigned char *byte_arr, int byte_len, char *hex_str) {
    int i;
    for (i = 0; i < byte_len; i++) {
        sprintf(hex_str + i * 2, "%02x", byte_arr[i]);
    }
}

// 使用AES-128-CBC加密
void aes_encrypt(const unsigned char *key, const unsigned char *iv, const unsigned char *input, int input_len, unsigned char *output) {
    AES_KEY aes_key;
    AES_set_encrypt_key(key, 128, &aes_key);
    AES_cbc_encrypt(input, output, input_len, &aes_key, iv, AES_ENCRYPT);
}

// 使用AES-128-CBC解密
void aes_decrypt(const unsigned char *key, const unsigned char *iv, const unsigned char *input, int input_len, unsigned char *output) {
    AES_KEY aes_key;
    AES_set_decrypt_key(key, 128, &aes_key);
    AES_cbc_encrypt(input, output, input_len, &aes_key, iv, AES_DECRYPT);
}