#include <string.h>
#include "H3QPack.h"
#include "Huffman/HuffmanCodec.h"

#define ToLower(c)   (uint8_t) ((c >= 'A' && c <= 'Z') ? (c | 0x20) : c)

static void StrLow(uint8_t *dst, uint8_t *src, size_t n)
{
    while (n) {
        *dst = ToLower(*src);
        dst++;
        src++;
        n--;
    }
}

uint8_t *H3EncodeVarlenInt(uint8_t *p, uint64_t value, uint64_t *bufLen)
{
    if (value <= 63) {
        if (p == NULL) {
            if (bufLen) {
                *bufLen = 1;
            }

            return NULL;
        }

        *p++ = value;
        return p;
    }

    if (value <= 16383) {
        if (p == NULL) {
            if (bufLen) {
                *bufLen = 2;
            }

            return NULL;
        }

        *p++ = 0x40 | (value >> 8);
        *p++ = value;
        return p;
    }

    if (value <= 1073741823) {
        if (p == NULL) {
            if (bufLen) {
                *bufLen = 4;
            }

            return NULL;
        }

        *p++ = 0x80 | (value >> 24);
        *p++ = (value >> 16);
        *p++ = (value >> 8);
        *p++ = value;
        return p;
    }

    if (p == NULL) {
        if (bufLen) {
            *bufLen = 8;
        }

        return NULL;
    }

    *p++ = 0xc0 | (value >> 56);
    *p++ = (value >> 48);
    *p++ = (value >> 40);
    *p++ = (value >> 32);
    *p++ = (value >> 24);
    *p++ = (value >> 16);
    *p++ = (value >> 8);
    *p++ = value;
    return p;
}

uint8_t *H3EncodePrefixInt(uint8_t *p, uint64_t value, uint32_t prefix, uint64_t *bufLen)
{
    uint32_t  thresh, n;

    thresh = (1 << prefix) - 1;

    if (value < thresh) {
        if (p == NULL) {
            if (bufLen) {
                *bufLen = 1;
            }

            return NULL;
        }

        *p++ |= value;
        return p;
    }

    value -= thresh;

    if (p == NULL) {
        for (n = 2; value >= 128; n++) {
            value >>= 7;
        }

        if (bufLen) {
            *bufLen = n;
        }

        return NULL;
    }

    *p++ |= thresh;

    while (value >= 128) {
        *p++ = 0x80 | value;
        value >>= 7;
    }

    *p++ = value;

    return p;
}

uint8_t *H3EncodeInsertNR(uint8_t *p, uint32_t index, uint8_t *data, size_t len, uint64_t *bufLen)
{
    size_t    hlen;
    uint8_t  *p1, *p2;
    uint64_t  idxLen, valLen;

    // TODO: dynamic

    if (p == NULL) {
        H3EncodePrefixInt(NULL, index, 6, &idxLen);
        H3EncodePrefixInt(NULL, len, 7, &valLen);

        if (bufLen) {
            *bufLen = idxLen + valLen + len;
        }

        return NULL;
    }

    *p = H3_ENCODER_INSTRUCTION_INR | 0x40;
    p = H3EncodePrefixInt(p, index, 6, NULL);

    p1 = p;
    *p = H3_CODEC_INSERT_VAL_UNKNOWN;
    p = H3EncodePrefixInt(p, len, 7, NULL);

    if (data) {
        p2 = p;
        hlen = HuffmanEncode(data, len, p, false);

        if (hlen) {
            p = p1;
            *p = H3_CODEC_INSERT_VAL_HUFFMAN;
            p = H3EncodePrefixInt(p, hlen, 7, NULL);

            if (p != p2) {
                memmove(p, p2, hlen);
            }

            p += hlen;

        } else {
            memcpy(p, data, len);
            p += len;
        }
    }

    return p;
}

uint8_t *H3EncodeInsertLN(uint8_t *p, uint8_t *name, size_t nameLen, uint8_t *val, size_t valLen, uint64_t *bufLen)
{
    size_t    hlen;
    uint8_t  *p1, *p2;
    uint64_t  len1, len2;

    // TODO: dynamic

    if (p == NULL) {
        H3EncodePrefixInt(NULL, nameLen, 5, &len1);
        H3EncodePrefixInt(NULL, valLen, 7, &len2);

        if (bufLen) {
            *bufLen = len1 + len2 + nameLen + valLen;
        }

        return NULL;
    }

    p1 = p;
    *p = H3_ENCODER_INSTRUCTION_ILN | H3_CODEC_INSERT_NAME_UNKNOWN;
    p = H3EncodePrefixInt(p, nameLen, 5, NULL);

    if (name) {
        p2 = p;
        hlen = HuffmanEncode(name, nameLen, p, false);

        if (hlen) {
            p = p1;
            *p = H3_ENCODER_INSTRUCTION_ILN | H3_CODEC_INSERT_NAME_HUFFMAN;
            p = H3EncodePrefixInt(p, hlen, 5, NULL);

            if (p != p2) {
                memmove(p, p2, hlen);
            }

            p += hlen;

        } else {
            memcpy(p, name, nameLen);
            p += nameLen;
        }
    }

    p1 = p;
    *p = H3_CODEC_INSERT_VAL_UNKNOWN;
    p = H3EncodePrefixInt(p, valLen, 7, NULL);

    if (val) {
        p2 = p;
        hlen = HuffmanEncode(val, valLen, p, false);

        if (hlen) {
            p = p1;
            *p = H3_CODEC_INSERT_VAL_HUFFMAN;
            p = H3EncodePrefixInt(p, hlen, 7, NULL);

            if (p != p2) {
                memmove(p, p2, hlen);
            }

            p += hlen;

        } else {
            memcpy(p, val, valLen);
            p += valLen;
        }
    }

    return p;
}

uint8_t *H3EncodeFieldSectionPrefix(uint8_t *p, uint32_t insertCount, bool sign, uint32_t deltaBase, uint64_t *bufLen)
{
    uint64_t  countLen, baseLen;

    if (p == NULL) {
        H3EncodePrefixInt(NULL, insertCount, 8, &countLen);
        H3EncodePrefixInt(NULL, deltaBase, 7, &baseLen);

        if (bufLen) {
            *bufLen = countLen + baseLen;
        }

        return NULL;
    }

    *p = 0;
    p = H3EncodePrefixInt(p, insertCount, 8, NULL);

    *p = sign ? 0x80 : 0;
    p = H3EncodePrefixInt(p, deltaBase, 7, NULL);

    return p;
}

uint8_t *H3EncodeFieldIFL(uint8_t *p, bool dynamic, uint32_t index, uint64_t *bufLen)
{
    /* Indexed Field Line */

    if (p == NULL) {
        H3EncodePrefixInt(NULL, index, 6, bufLen);
        return NULL;
    }

    *p = dynamic ? 0x80 : 0xc0;

    return H3EncodePrefixInt(p, index, 6, NULL);
}

uint8_t *H3EncodeFieldLFLNR(uint8_t *p, bool dynamic, uint32_t index, uint8_t *data, size_t len, uint64_t *bufLen)
{
    size_t    hlen;
    uint8_t  *p1, *p2;
    uint64_t  idxLen, valLen;

    /* Literal Field Line With Name Reference */

    if (p == NULL) {
        H3EncodePrefixInt(NULL, index, 4, &idxLen);
        H3EncodePrefixInt(NULL, len, 7, &valLen);

        if (bufLen) {
            *bufLen = idxLen + valLen + len;
        }

        return NULL;
    }

    *p = dynamic ? 0x40 : 0x50;
    p = H3EncodePrefixInt(p, index, 4, NULL);

    p1 = p;
    *p = 0;
    p = H3EncodePrefixInt(p, len, 7, NULL);

    if (data) {
        p2 = p;
        hlen = HuffmanEncode(data, len, p, false);

        if (hlen) {
            p = p1;
            *p = 0x80;
            p = H3EncodePrefixInt(p, hlen, 7, NULL);

            if (p != p2) {
                memmove(p, p2, hlen);
            }

            p += hlen;

        } else {
            memcpy(p, data, len);
            p += len;
        }
    }

    return p;
}

uint8_t *H3EncodeFieldLFLLN(uint8_t *p, H3String *name, H3String *value, uint64_t *bufLen)
{
    size_t    hlen;
    uint8_t  *p1, *p2;
    uint64_t  nameLen, valLen;

    /* Literal Field Line With Literal Name */

    if (p == NULL) {
        H3EncodePrefixInt(NULL, name->len, 3, &nameLen);
        H3EncodePrefixInt(NULL, value->len, 7, &valLen);

        if (bufLen) {
            *bufLen = nameLen + valLen + name->len + value->len;
        }

        return NULL;
    }

    p1 = p;
    *p = 0x20;
    p = H3EncodePrefixInt(p, name->len, 3, NULL);

    p2 = p;
    hlen = HuffmanEncode(name->data, name->len, p, true);

    if (hlen) {
        p = p1;
        *p = 0x28;
        p = H3EncodePrefixInt(p, hlen, 3, NULL);

        if (p != p2) {
            memmove(p, p2, hlen);
        }

        p += hlen;

    } else {
        StrLow(p, name->data, name->len);
        p += name->len;
    }

    p1 = p;
    *p = 0;
    p = H3EncodePrefixInt(p, value->len, 7, NULL);

    p2 = p;
    hlen = HuffmanEncode(value->data, value->len, p, false);

    if (hlen) {
        p = p1;
        *p = 0x80;
        p = H3EncodePrefixInt(p, hlen, 7, NULL);

        if (p != p2) {
            memmove(p, p2, hlen);
        }

        p += hlen;

    } else {
        memcpy(p, value->data, value->len);
        p += value->len;
    }

    return p;
}

uint8_t *H3EncodeFieldIFLPBI(uint8_t *p, uint32_t index, uint64_t *bufLen)
{
    /* Indexed Field Line With Post-Base Index */

    if (p == NULL) {
        H3EncodePrefixInt(NULL, index, 4, bufLen);
        return NULL;
    }

    *p = 0x10;

    return H3EncodePrefixInt(p, index, 4, NULL);
}

uint8_t *H3EncodeFieldLFLPBNR(uint8_t *p, uint32_t index, uint8_t *data, size_t len, uint64_t *bufLen)
{
    size_t    hlen;
    uint8_t  *p1, *p2;
    uint64_t  idxLen, baseLen;

    /* Literal Field Line With Post-Base Name Reference */

    if (p == NULL) {
        H3EncodePrefixInt(NULL, index, 3, &idxLen);
        H3EncodePrefixInt(NULL, len, 7, &baseLen);

        if (bufLen) {
            *bufLen = idxLen + baseLen + len;
        }

        return NULL;
    }

    *p = 0;
    p = H3EncodePrefixInt(p, index, 3, NULL);

    p1 = p;
    *p = 0;
    p = H3EncodePrefixInt(p, len, 7, NULL);

    if (data) {
        p2 = p;
        hlen = HuffmanEncode(data, len, p, false);

        if (hlen) {
            p = p1;
            *p = 0x80;
            p = H3EncodePrefixInt(p, hlen, 7, NULL);

            if (p != p2) {
                memmove(p, p2, hlen);
            }

            p += hlen;

        } else {
            memcpy(p, data, len);
            p += len;
        }
    }

    return p;
}
