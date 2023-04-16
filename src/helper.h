#pragma once

#include "message.h"
#include <stddef.h>

size_t mqtt_decodeHex(const char *hexstr, unsigned char *buf, size_t bufsize);
size_t mqtt_encodeHex(const unsigned char *buf, size_t buflen, char *hexstr, size_t hexsize);
size_t base64_encoded_len(size_t n);
size_t base64_encode(char *dst, const unsigned char *src, size_t len);
int fmt_RexProperty(char *buf, int len, const RexProperty_t *v);
int fmt_RexEvent(char *buf, int len, const RexEvent_t *v);
int fmt_RexService(char *buf, int len, const RexService_t *v);
int fmt_RexMessage(char *buf, int len, const RexMessage_t *v);
