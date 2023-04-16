#include "helper.h"

static const char *encodeURL = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";

size_t base64_encoded_len(size_t n) {
    return (n * 8 + 5) / 6;
}

size_t base64_encode(char *dst, const unsigned char *src, size_t len) {
    if (len == 0) {
        return 0;
    }
    int                  n      = (len / 3) * 3;
    const unsigned char *end    = src + n;
    size_t               remain = len - n;
    char                *start  = dst;
    while (src < end) {
        unsigned val = ((unsigned)src[0] << 16) | ((unsigned)src[1] << 8) | (unsigned)src[2];
        *dst++       = encodeURL[(val >> 18) & 0x3F];
        *dst++       = encodeURL[(val >> 12) & 0x3F];
        *dst++       = encodeURL[(val >> 6) & 0x3F];
        *dst++       = encodeURL[val & 0x3F];
        src += 3;
    }
    if (remain > 0) {
        unsigned val = (unsigned)src[0] << 16;
        if (remain == 2) {
            val |= (unsigned)src[1] << 8;
        }
        *dst++ = encodeURL[(val >> 18) & 0x3F];
        *dst++ = encodeURL[(val >> 12) & 0x3F];
        if (remain == 2) {
            *dst++ = encodeURL[(val >> 6) & 0x3F];
        }
    }
    return dst - start;
}
