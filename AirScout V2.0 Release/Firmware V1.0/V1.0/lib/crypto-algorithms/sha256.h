/*********************************************************************
 * Filename:   sha256.h
 * Author:     Brad Conte (brad AT bradconte.com)
 * Copyright:
 * Disclaimer: This code is presented "as is" without any guarantees.
 * Details:    Defines the API for the corresponding SHA1 implementation.
 *********************************************************************/

#ifndef CRYPTO_ALGORITHMS_SHA256_H
#define CRYPTO_ALGORITHMS_SHA256_H

#ifndef SHA256_H
#define SHA256_H

/*************************** HEADER FILES ***************************/
#include <stddef.h>
#include <stdint.h>
/****************************** MACROS ******************************/
#define SHA256_BLOCK_SIZE 32  // SHA256 outputs a 32 byte digest

/**************************** DATA TYPES ****************************/
typedef unsigned char BYTE;     // 8-bit byte
typedef uint32_t _SHA256_WORD;  // 32-bit word, change to "long" for 16-bit machines

typedef struct {
    BYTE data[64];
    _SHA256_WORD datalen;
    unsigned long long bitlen;
    _SHA256_WORD state[8];
} SHA256_CTX;

/*********************** FUNCTION DECLARATIONS **********************/
void sha256_init(SHA256_CTX *ctx);
void sha256_update(SHA256_CTX *ctx, const char *data, size_t len);
void sha256_final(SHA256_CTX *ctx, uint8_t *hash);

#endif  // SHA256_H

#endif  // CRYPTO_ALGORITHMS_SHA256_H
