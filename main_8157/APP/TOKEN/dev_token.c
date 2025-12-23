/**
 * Copyright (c), 2012~2020 iot.10086.cn All Rights Reserved
 *
 * @file dev_token.c
 * @brief
 */

/*****************************************************************************/
/* Includes                                                                  */
/*****************************************************************************/
#include "dev_token.h"

/*****************************************************************************/
/* Local Definitions ( Constant and Macro )                                  */
/*****************************************************************************/
#define DEV_TOKEN_LEN 256
#define DEV_TOKEN_VERISON_STR "2018-10-31"

#define DEV_TOKEN_SIG_METHOD_MD5 "md5"

#define XMEMCPY(d,s,l)    memcpy((d),(s),(l))
#define XMEMSET(b,c,l)    memset((b),(c),(l))

#define min(a,b) ((a)<(b)?(a):(b))

#define XTRANSFORM(S,B)  Transform((S))

#define F1(x, y, z) (z ^ (x & (y ^ z)))
#define F2(x, y, z) F1(z, x, y)
#define F3(x, y, z) (x ^ y ^ z)
#define F4(x, y, z) (y ^ (x | ~z))

#define MD5STEP(f, w, x, y, z, data, s) w = rotlFixed(w + f(x, y, z) + data, s) + x
/*****************************************************************************/
/* Structures, Enum and Typedefs                                             */
/*****************************************************************************/

/*****************************************************************************/
/* Local Function Prototype                                                  */
/*****************************************************************************/

/*****************************************************************************/
/* Local Variables                                                           */
/*****************************************************************************/

/*****************************************************************************/
/* Global Variables                                                          */
/*****************************************************************************/

/*****************************************************************************/
/* Function Implementation                                                   */
/*****************************************************************************/
enum
{
	BAD         = 0xFF,  /* invalid encoding */
	PAD         = '=',
	PEM_LINE_SZ = 64
};

enum {
    IPAD    = 0x36,
    OPAD    = 0x5C,
};

static const byte base64Decode[] = {
	62, BAD, BAD, BAD, 63,   /* + starts at 0x2B */
	52, 53, 54, 55, 56, 57, 58, 59, 60, 61,
	BAD, BAD, BAD, BAD, BAD, BAD, BAD,
	0, 1, 2, 3, 4, 5, 6, 7, 8, 9,
	10, 11, 12, 13, 14, 15, 16, 17, 18, 19,
	20, 21, 22, 23, 24, 25,
	BAD, BAD, BAD, BAD, BAD, BAD,
	26, 27, 28, 29, 30, 31, 32, 33, 34, 35,
	36, 37, 38, 39, 40, 41, 42, 43, 44, 45,
	46, 47, 48, 49, 50, 51
};

int Base64_Decode(const byte* in, word32 inLen, byte* out, word32* outLen)
{
    word32 i = 0;
    word32 j = 0;
    word32 plainSz = inLen - ((inLen + (PEM_LINE_SZ - 1)) / PEM_LINE_SZ );
    const byte maxIdx = (byte)sizeof(base64Decode) + 0x2B - 1;

    plainSz = (plainSz * 3 + 3) / 4;
    if (plainSz > *outLen) return BAD_FUNC_ARG;

    while (inLen > 3) {
        byte b1, b2, b3;
        byte e1 = in[j++];
        byte e2 = in[j++];
        byte e3 = in[j++];
        byte e4 = in[j++];

        int pad3 = 0;
        int pad4 = 0;

        if (e1 == 0)            /* end file 0's */
            break;
        if (e3 == PAD)
            pad3 = 1;
        if (e4 == PAD)
            pad4 = 1;

        if (e1 < 0x2B || e2 < 0x2B || e3 < 0x2B || e4 < 0x2B) {
//            WOLFSSL_MSG("Bad Base64 Decode data, too small");
            return ASN_INPUT_E;
        }

        if (e1 > maxIdx || e2 > maxIdx || e3 > maxIdx || e4 > maxIdx) {
//            WOLFSSL_MSG("Bad Base64 Decode data, too big");
            return ASN_INPUT_E;
        }

        e1 = base64Decode[e1 - 0x2B];
        e2 = base64Decode[e2 - 0x2B];
        e3 = (e3 == PAD) ? 0 : base64Decode[e3 - 0x2B];
        e4 = (e4 == PAD) ? 0 : base64Decode[e4 - 0x2B];

        b1 = (byte)((e1 << 2) | (e2 >> 4));
        b2 = (byte)(((e2 & 0xF) << 4) | (e3 >> 2));
        b3 = (byte)(((e3 & 0x3) << 6) | e4);

        out[i++] = b1;
        if (!pad3)
            out[i++] = b2;
        if (!pad4)
            out[i++] = b3;
        else
            break;

        inLen -= 4;
        if (inLen && (in[j] == ' ' || in[j] == '\r' || in[j] == '\n')) {
            byte endLine = in[j++];
            inLen--;
            while (inLen && endLine == ' ') {   /* allow trailing whitespace */
                endLine = in[j++];
                inLen--;
            }
            if (endLine == '\r') {
                if (inLen) {
                    endLine = in[j++];
                    inLen--;
                }
            }
            if (endLine != '\n') {
//                WOLFSSL_MSG("Bad end of line in Base64 Decode");
                return ASN_INPUT_E;
            }
        }
    }
    *outLen = i;

    return 0;
}

static int _InitMd5(wc_Md5* md5)
{
    int ret = 0;

    md5->digest[0] = 0x67452301L;
    md5->digest[1] = 0xefcdab89L;
    md5->digest[2] = 0x98badcfeL;
    md5->digest[3] = 0x10325476L;

    md5->buffLen = 0;
    md5->loLen   = 0;
    md5->hiLen   = 0;

    return ret;
}

int wc_InitMd5_ex(wc_Md5* md5, void* heap, int devId)
{
    int ret = 0;

    if (md5 == NULL)
        return BAD_FUNC_ARG;

    md5->heap = heap;

    ret = _InitMd5(md5);
    if (ret != 0)
        return ret;

    (void)devId;

    return ret;
}

int wc_InitMd5(wc_Md5* md5)
{
    if (md5 == NULL) {
        return BAD_FUNC_ARG;
    }
    return wc_InitMd5_ex(md5, NULL, -2);
}

int _InitHmac(Hmac* hmac, int type, void* heap)
{
    int ret = 0;

    switch (type) {
        case WC_MD5:
            ret = wc_InitMd5(&hmac->hash.md5);
            break;
        default:
            ret = BAD_FUNC_ARG;
            break;
    }

    hmac->heap = heap;

    return ret;
}

word32 rotlFixed(word32 x, word32 y)
{
	return (x << y) | (x >> (sizeof(y) * 8 - y));
}

word32 rotrFixed(word32 x, word32 y)
{
	return (x >> y) | (x << (sizeof(y) * 8 - y));
}

static int Transform(wc_Md5* md5)
{
	/* Copy context->state[] to working vars  */
	word32 a = md5->digest[0];
	word32 b = md5->digest[1];
	word32 c = md5->digest[2];
	word32 d = md5->digest[3];

	MD5STEP(F1, a, b, c, d, md5->buffer[0]  + 0xd76aa478,  7);
	MD5STEP(F1, d, a, b, c, md5->buffer[1]  + 0xe8c7b756, 12);
	MD5STEP(F1, c, d, a, b, md5->buffer[2]  + 0x242070db, 17);
	MD5STEP(F1, b, c, d, a, md5->buffer[3]  + 0xc1bdceee, 22);
	MD5STEP(F1, a, b, c, d, md5->buffer[4]  + 0xf57c0faf,  7);
	MD5STEP(F1, d, a, b, c, md5->buffer[5]  + 0x4787c62a, 12);
	MD5STEP(F1, c, d, a, b, md5->buffer[6]  + 0xa8304613, 17);
	MD5STEP(F1, b, c, d, a, md5->buffer[7]  + 0xfd469501, 22);
	MD5STEP(F1, a, b, c, d, md5->buffer[8]  + 0x698098d8,  7);
	MD5STEP(F1, d, a, b, c, md5->buffer[9]  + 0x8b44f7af, 12);
	MD5STEP(F1, c, d, a, b, md5->buffer[10] + 0xffff5bb1, 17);
	MD5STEP(F1, b, c, d, a, md5->buffer[11] + 0x895cd7be, 22);
	MD5STEP(F1, a, b, c, d, md5->buffer[12] + 0x6b901122,  7);
	MD5STEP(F1, d, a, b, c, md5->buffer[13] + 0xfd987193, 12);
	MD5STEP(F1, c, d, a, b, md5->buffer[14] + 0xa679438e, 17);
	MD5STEP(F1, b, c, d, a, md5->buffer[15] + 0x49b40821, 22);

	MD5STEP(F2, a, b, c, d, md5->buffer[1]  + 0xf61e2562,  5);
	MD5STEP(F2, d, a, b, c, md5->buffer[6]  + 0xc040b340,  9);
	MD5STEP(F2, c, d, a, b, md5->buffer[11] + 0x265e5a51, 14);
	MD5STEP(F2, b, c, d, a, md5->buffer[0]  + 0xe9b6c7aa, 20);
	MD5STEP(F2, a, b, c, d, md5->buffer[5]  + 0xd62f105d,  5);
	MD5STEP(F2, d, a, b, c, md5->buffer[10] + 0x02441453,  9);
	MD5STEP(F2, c, d, a, b, md5->buffer[15] + 0xd8a1e681, 14);
	MD5STEP(F2, b, c, d, a, md5->buffer[4]  + 0xe7d3fbc8, 20);
	MD5STEP(F2, a, b, c, d, md5->buffer[9]  + 0x21e1cde6,  5);
	MD5STEP(F2, d, a, b, c, md5->buffer[14] + 0xc33707d6,  9);
	MD5STEP(F2, c, d, a, b, md5->buffer[3]  + 0xf4d50d87, 14);
	MD5STEP(F2, b, c, d, a, md5->buffer[8]  + 0x455a14ed, 20);
	MD5STEP(F2, a, b, c, d, md5->buffer[13] + 0xa9e3e905,  5);
	MD5STEP(F2, d, a, b, c, md5->buffer[2]  + 0xfcefa3f8,  9);
	MD5STEP(F2, c, d, a, b, md5->buffer[7]  + 0x676f02d9, 14);
	MD5STEP(F2, b, c, d, a, md5->buffer[12] + 0x8d2a4c8a, 20);

	MD5STEP(F3, a, b, c, d, md5->buffer[5]  + 0xfffa3942,  4);
	MD5STEP(F3, d, a, b, c, md5->buffer[8]  + 0x8771f681, 11);
	MD5STEP(F3, c, d, a, b, md5->buffer[11] + 0x6d9d6122, 16);
	MD5STEP(F3, b, c, d, a, md5->buffer[14] + 0xfde5380c, 23);
	MD5STEP(F3, a, b, c, d, md5->buffer[1]  + 0xa4beea44,  4);
	MD5STEP(F3, d, a, b, c, md5->buffer[4]  + 0x4bdecfa9, 11);
	MD5STEP(F3, c, d, a, b, md5->buffer[7]  + 0xf6bb4b60, 16);
	MD5STEP(F3, b, c, d, a, md5->buffer[10] + 0xbebfbc70, 23);
	MD5STEP(F3, a, b, c, d, md5->buffer[13] + 0x289b7ec6,  4);
	MD5STEP(F3, d, a, b, c, md5->buffer[0]  + 0xeaa127fa, 11);
	MD5STEP(F3, c, d, a, b, md5->buffer[3]  + 0xd4ef3085, 16);
	MD5STEP(F3, b, c, d, a, md5->buffer[6]  + 0x04881d05, 23);
	MD5STEP(F3, a, b, c, d, md5->buffer[9]  + 0xd9d4d039,  4);
	MD5STEP(F3, d, a, b, c, md5->buffer[12] + 0xe6db99e5, 11);
	MD5STEP(F3, c, d, a, b, md5->buffer[15] + 0x1fa27cf8, 16);
	MD5STEP(F3, b, c, d, a, md5->buffer[2]  + 0xc4ac5665, 23);

	MD5STEP(F4, a, b, c, d, md5->buffer[0]  + 0xf4292244,  6);
	MD5STEP(F4, d, a, b, c, md5->buffer[7]  + 0x432aff97, 10);
	MD5STEP(F4, c, d, a, b, md5->buffer[14] + 0xab9423a7, 15);
	MD5STEP(F4, b, c, d, a, md5->buffer[5]  + 0xfc93a039, 21);
	MD5STEP(F4, a, b, c, d, md5->buffer[12] + 0x655b59c3,  6);
	MD5STEP(F4, d, a, b, c, md5->buffer[3]  + 0x8f0ccc92, 10);
	MD5STEP(F4, c, d, a, b, md5->buffer[10] + 0xffeff47d, 15);
	MD5STEP(F4, b, c, d, a, md5->buffer[1]  + 0x85845dd1, 21);
	MD5STEP(F4, a, b, c, d, md5->buffer[8]  + 0x6fa87e4f,  6);
	MD5STEP(F4, d, a, b, c, md5->buffer[15] + 0xfe2ce6e0, 10);
	MD5STEP(F4, c, d, a, b, md5->buffer[6]  + 0xa3014314, 15);
	MD5STEP(F4, b, c, d, a, md5->buffer[13] + 0x4e0811a1, 21);
	MD5STEP(F4, a, b, c, d, md5->buffer[4]  + 0xf7537e82,  6);
	MD5STEP(F4, d, a, b, c, md5->buffer[11] + 0xbd3af235, 10);
	MD5STEP(F4, c, d, a, b, md5->buffer[2]  + 0x2ad7d2bb, 15);
	MD5STEP(F4, b, c, d, a, md5->buffer[9]  + 0xeb86d391, 21);

	/* Add the working vars back into digest state[]  */
	md5->digest[0] += a;
	md5->digest[1] += b;
	md5->digest[2] += c;
	md5->digest[3] += d;

	return 0;
}

void AddLength(wc_Md5* md5, word32 len)
{
    word32 tmp = md5->loLen;
    if ((md5->loLen += len) < tmp) {
        md5->hiLen++;                       /* carry low to high */
    }
}

int wc_Md5Update(wc_Md5* md5, const byte* data, word32 len)
{
    int ret = 0;
    byte* local;

    if (md5 == NULL || (data == NULL && len > 0)) {
        return BAD_FUNC_ARG;
    }

    /* do block size increments */
    local = (byte*)md5->buffer;

    /* check that internal buffLen is valid */
    if (md5->buffLen >= WC_MD5_BLOCK_SIZE)
        return BUFFER_E;

    while (len) {
        word32 add = min(len, WC_MD5_BLOCK_SIZE - md5->buffLen);
        XMEMCPY(&local[md5->buffLen], data, add);

        md5->buffLen += add;
        data         += add;
        len          -= add;

        if (md5->buffLen == WC_MD5_BLOCK_SIZE) {
            XTRANSFORM(md5, local);
            AddLength(md5, WC_MD5_BLOCK_SIZE);
            md5->buffLen = 0;
        }
    }
    return ret;
}

int wc_Md5Final(wc_Md5* md5, byte* hash)
{
    byte* local;

    if (md5 == NULL || hash == NULL) {
        return BAD_FUNC_ARG;
    }

    local = (byte*)md5->buffer;

    AddLength(md5, md5->buffLen);  /* before adding pads */
    local[md5->buffLen++] = 0x80;  /* add 1 */

    /* pad with zeros */
    if (md5->buffLen > WC_MD5_PAD_SIZE) {
        XMEMSET(&local[md5->buffLen], 0, WC_MD5_BLOCK_SIZE - md5->buffLen);
        md5->buffLen += WC_MD5_BLOCK_SIZE - md5->buffLen;

        XTRANSFORM(md5, local);
        md5->buffLen = 0;
    }
    XMEMSET(&local[md5->buffLen], 0, WC_MD5_PAD_SIZE - md5->buffLen);

    /* put lengths in bits */
    md5->hiLen = (md5->loLen >> (8*sizeof(md5->loLen) - 3)) +
                 (md5->hiLen << 3);
    md5->loLen = md5->loLen << 3;

    /* store lengths */
    /* ! length ordering dependent on digest endian type ! */
    XMEMCPY(&local[WC_MD5_PAD_SIZE], &md5->loLen, sizeof(word32));
    XMEMCPY(&local[WC_MD5_PAD_SIZE + sizeof(word32)], &md5->hiLen, sizeof(word32));

    /* final transform and result to hash */
    XTRANSFORM(md5, local);

    XMEMCPY(hash, md5->digest, WC_MD5_DIGEST_SIZE);

    return _InitMd5(md5); /* reset state */
}

int wc_HmacSetKey(Hmac* hmac, int type, const byte* key, word32 length)
{
    byte*  ip;
    byte*  op;
    word32 i, hmac_block_size = 0;
    int    ret = 0;
    void*  heap = NULL;

    hmac->innerHashKeyed = 0;
    hmac->macType = (byte)type;

    ret = _InitHmac(hmac, type, heap);
    if (ret != 0)
        return ret;

    ip = (byte*)hmac->ipad;
    op = (byte*)hmac->opad;

    switch (hmac->macType) {
        case WC_MD5:
            hmac_block_size = WC_MD5_BLOCK_SIZE;
            if (length <= WC_MD5_BLOCK_SIZE) {
                if (key != NULL) {
                    XMEMCPY(ip, key, length);
                }
            }
            else {
                ret = wc_Md5Update(&hmac->hash.md5, key, length);
                if (ret != 0)
                    break;
                ret = wc_Md5Final(&hmac->hash.md5, ip);
                if (ret != 0)
                    break;
                length = WC_MD5_DIGEST_SIZE;
            }
            break;
        default:
            return BAD_FUNC_ARG;
    }

    if (ret == 0) {
        if (length < hmac_block_size)
            XMEMSET(ip + length, 0, hmac_block_size - length);

        for(i = 0; i < hmac_block_size; i++) {
            op[i] = ip[i] ^ OPAD;
            ip[i] ^= IPAD;
        }
    }

    return ret;
}

static int HmacKeyInnerHash(Hmac* hmac)
{
    int ret = 0;

    switch (hmac->macType) {
        case WC_MD5:
            ret = wc_Md5Update(&hmac->hash.md5, (byte*)hmac->ipad, WC_MD5_BLOCK_SIZE);
            break;
        default:
            break;
    }

    if (ret == 0)
        hmac->innerHashKeyed = 1;

    return ret;
}

int wc_HmacUpdate(Hmac* hmac, const byte* msg, word32 length)
{
    int ret = 0;

    if (hmac == NULL || (msg == NULL && length > 0)) {
        return BAD_FUNC_ARG;
    }

    if (!hmac->innerHashKeyed) {
        ret = HmacKeyInnerHash(hmac);
        if (ret != 0)
            return ret;
    }

    switch (hmac->macType) {
        case WC_MD5:
            ret = wc_Md5Update(&hmac->hash.md5, msg, length);
            break;
        default:
            break;
    }

    return ret;
}

int wc_HmacFinal(Hmac* hmac, byte* hash)
{
    int ret;

    if (hmac == NULL || hash == NULL) {
        return BAD_FUNC_ARG;
    }

    if (!hmac->innerHashKeyed) {
        ret = HmacKeyInnerHash(hmac);
        if (ret != 0)
            return ret;
    }

    switch (hmac->macType) {
        case WC_MD5:
            ret = wc_Md5Final(&hmac->hash.md5, (byte*)hmac->innerHash);
            if (ret != 0)
                break;
            ret = wc_Md5Update(&hmac->hash.md5, (byte*)hmac->opad,
                                                                WC_MD5_BLOCK_SIZE);
            if (ret != 0)
                break;
            ret = wc_Md5Update(&hmac->hash.md5, (byte*)hmac->innerHash,
                                                               WC_MD5_DIGEST_SIZE);
            if (ret != 0)
                break;
            ret = wc_Md5Final(&hmac->hash.md5, hash);
            break;
        default:
            ret = BAD_FUNC_ARG;
            break;
    }

    if (ret == 0) {
        hmac->innerHashKeyed = 0;
    }

    return ret;
}

enum Escaped {
        WC_STD_ENC = 0,       /* normal \n line ending encoding */
        WC_ESC_NL_ENC,        /* use escape sequence encoding   */
        WC_NO_NL_ENC          /* no encoding at all             */
    }; /* Encoding types */

static
const byte base64Encode[] = { 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J',
                              'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T',
                              'U', 'V', 'W', 'X', 'Y', 'Z',
                              'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j',
                              'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't',
                              'u', 'v', 'w', 'x', 'y', 'z',
                              '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
                              '+', '/'
                            };	

/* make sure *i (idx) won't exceed max, store and possibly escape to out,
 * raw means use e w/o decode,  0 on success */
static int CEscape(int escaped, byte e, byte* out, word32* i, word32 max,
                  int raw, int getSzOnly)
{
    int    doEscape = 0;
    word32 needed = 1;
    word32 idx = *i;

    byte basic;
    byte plus    = 0;
    byte equals  = 0;
    byte newline = 0;

    if (raw)
        basic = e;
    else
        basic = base64Encode[e];

    /* check whether to escape. Only escape for EncodeEsc */
    if (escaped == WC_ESC_NL_ENC) {
        switch ((char)basic) {
            case '+' :
                plus     = 1;
                doEscape = 1;
                needed  += 2;
                break;
            case '=' :
                equals   = 1;
                doEscape = 1;
                needed  += 2;
                break;
            case '\n' :
                newline  = 1;
                doEscape = 1;
                needed  += 2;
                break;
            default:
                /* do nothing */
                break;
        }
    }

    /* check size */
    if ( (idx+needed) > max && !getSzOnly) {
//        WOLFSSL_MSG("Escape buffer max too small");
        return BUFFER_E;
    }

    /* store it */
    if (doEscape == 0) {
        if(getSzOnly)
            idx++;
        else
            out[idx++] = basic;
    }
    else {
        if(getSzOnly)
            idx+=3;
        else {
            out[idx++] = '%';  /* start escape */

            if (plus) {
                out[idx++] = '2';
                out[idx++] = 'B';
            }
            else if (equals) {
                out[idx++] = '3';
                out[idx++] = 'D';
            }
            else if (newline) {
                out[idx++] = '0';
                out[idx++] = 'A';
            }
        }
    }
    *i = idx;

    return 0;
}
	
/* internal worker, handles both escaped and normal line endings.
   If out buffer is NULL, will return sz needed in outLen */
static int DoBase64_Encode(const byte* in, word32 inLen, byte* out,
                           word32* outLen, int escaped)
{
    int    ret = 0;
    word32 i = 0,
           j = 0,
           n = 0;   /* new line counter */

    int    getSzOnly = (out == NULL);

    word32 outSz = (inLen + 3 - 1) / 3 * 4;
    word32 addSz = (outSz + PEM_LINE_SZ - 1) / PEM_LINE_SZ;  /* new lines */

    if (escaped == WC_ESC_NL_ENC)
        addSz *= 3;   /* instead of just \n, we're doing %0A triplet */
    else if (escaped == WC_NO_NL_ENC)
        addSz = 0;    /* encode without \n */

    outSz += addSz;

    /* if escaped we can't predetermine size for one pass encoding, but
     * make sure we have enough if no escapes are in input
     * Also need to ensure outLen valid before dereference */
    if (!outLen || (outSz > *outLen && !getSzOnly)) return BAD_FUNC_ARG;

    while (inLen > 2) {
        byte b1 = in[j++];
        byte b2 = in[j++];
        byte b3 = in[j++];

        /* encoded idx */
        byte e1 = b1 >> 2;
        byte e2 = (byte)(((b1 & 0x3) << 4) | (b2 >> 4));
        byte e3 = (byte)(((b2 & 0xF) << 2) | (b3 >> 6));
        byte e4 = b3 & 0x3F;

        /* store */
        ret = CEscape(escaped, e1, out, &i, *outLen, 0, getSzOnly);
        if (ret != 0) break;
        ret = CEscape(escaped, e2, out, &i, *outLen, 0, getSzOnly);
        if (ret != 0) break;
        ret = CEscape(escaped, e3, out, &i, *outLen, 0, getSzOnly);
        if (ret != 0) break;
        ret = CEscape(escaped, e4, out, &i, *outLen, 0, getSzOnly);
        if (ret != 0) break;

        inLen -= 3;

        /* Insert newline after PEM_LINE_SZ, unless no \n requested */
        if (escaped != WC_NO_NL_ENC && (++n % (PEM_LINE_SZ/4)) == 0 && inLen){
            ret = CEscape(escaped, '\n', out, &i, *outLen, 1, getSzOnly);
            if (ret != 0) break;
        }
    }

    /* last integral */
    if (inLen && ret == 0) {
        int twoBytes = (inLen == 2);

        byte b1 = in[j++];
        byte b2 = (twoBytes) ? in[j++] : 0;

        byte e1 = b1 >> 2;
        byte e2 = (byte)(((b1 & 0x3) << 4) | (b2 >> 4));
        byte e3 = (byte)((b2 & 0xF) << 2);

        ret = CEscape(escaped, e1, out, &i, *outLen, 0, getSzOnly);
        if (ret == 0)
            ret = CEscape(escaped, e2, out, &i, *outLen, 0, getSzOnly);
        if (ret == 0) {
            /* third */
            if (twoBytes)
                ret = CEscape(escaped, e3, out, &i, *outLen, 0, getSzOnly);
            else
                ret = CEscape(escaped, '=', out, &i, *outLen, 1, getSzOnly);
        }
        /* fourth always pad */
        if (ret == 0)
            ret = CEscape(escaped, '=', out, &i, *outLen, 1, getSzOnly);
    }

    if (ret == 0 && escaped != WC_NO_NL_ENC)
        ret = CEscape(escaped, '\n', out, &i, *outLen, 1, getSzOnly);

    if (i != outSz && escaped != 1 && ret == 0)
        return ASN_INPUT_E;

    *outLen = i;
    if(ret == 0)
        return getSzOnly ? LENGTH_ONLY_E : 0;
    return ret;
}

int Base64_Encode_NoNl(const byte* in, word32 inLen, byte* out, word32* outLen)
{
    return DoBase64_Encode(in, inLen, out, outLen, WC_NO_NL_ENC);
}

int32_t dev_token_generate(char *token, enum sig_method_e method, uint32_t exp_time, const char *product_id,
                           const char *dev_name, const char *access_key)
{
    char base64_data[64] = { 0 };
    char str_for_sig[64] = { 0 };
    char sign_buf[128] = { 0 };
    uint32_t base64_data_len = sizeof(base64_data);
    char *sig_method_str = NULL;
    uint32_t sign_len = 0;
    char i = 0;
    char *tmp = NULL;
    Hmac hmac;

    sprintf(token, "version=%s", DEV_TOKEN_VERISON_STR);

    sprintf(token + strlen(token), "&res=products%%2F%s%%2Fdevices%%2F%s", product_id, dev_name);

    sprintf(token + strlen(token), "&et=%u", exp_time);

    Base64_Decode((byte*)access_key, strlen(access_key), (byte*)base64_data, &base64_data_len); // base64解码计算

    if(SIG_METHOD_MD5 == method)  // # 签名方法，支持md5、sha1、sha256
    {
        wc_HmacSetKey(&hmac, MD5, (byte*)base64_data, base64_data_len);
        sig_method_str = DEV_TOKEN_SIG_METHOD_MD5;
        sign_len = 16;
    }

    sprintf(token + strlen(token), "&method=%s", sig_method_str);

    sprintf(str_for_sig, "%u\n%s\nproducts/%s/devices/%s\n%s", exp_time, sig_method_str, product_id, dev_name, DEV_TOKEN_VERISON_STR);
    wc_HmacUpdate(&hmac, (byte*)str_for_sig, strlen(str_for_sig));
    wc_HmacFinal(&hmac, (byte*)sign_buf);

    memset(base64_data, 0, sizeof(base64_data));
    base64_data_len = sizeof(base64_data);
    Base64_Encode_NoNl((byte*)sign_buf, sign_len, (byte*)base64_data, &base64_data_len);

    strcat(token, "&sign=");
    tmp = token + strlen(token);

    for(i = 0; i < base64_data_len; i++)
    {
        switch(base64_data[i])
        {
        case '+':
            strcat(tmp, "%2B");
            tmp += 3;
            break;
        case ' ':
            strcat(tmp, "%20");
            tmp += 3;
            break;
        case '/':
            strcat(tmp, "%2F");
            tmp += 3;
            break;
        case '?':
            strcat(tmp, "%3F");
            tmp += 3;
            break;
        case '%':
            strcat(tmp, "%25");
            tmp += 3;
            break;
        case '#':
            strcat(tmp, "%23");
            tmp += 3;
            break;
        case '&':
            strcat(tmp, "%26");
            tmp += 3;
            break;
        case '=':
            strcat(tmp, "%3D");
            tmp += 3;
            break;
        default:
            *tmp = base64_data[i];
            tmp += 1;
            break;
        }
    }

    return 0;
}
