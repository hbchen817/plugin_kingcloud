#include "hmac.h"
#include <openssl/hmac.h>
#include <string.h>

int main(int argc, char **argv) {
    OpenSSL_add_all_digests();
    const char *msg = "oKO5YzSy040D84FFFE41AF46b17wpNLgyIe123";
    const char *key = "myYMO7mFHBPe7YMB";

    if (argc > 1) {
        msg = argv[1];
    }
    const char *methods[] = {
        "SHA1",
        "SHA224",
        "SHA256",
        "SHA384",
        "SHA512",
        "MD5"
    };
    for (int i = 0; i < sizeof(methods) / sizeof(methods[0]); i++) {
        char *digest = hmac(methods[i], msg, strlen(msg), key, strlen(key));
        printf("%s %s\n", methods[i], digest);
        free(digest);
    }
    EVP_cleanup();
    return 0;
}