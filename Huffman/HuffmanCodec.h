#ifndef __H3HUFF_CODEC_H__
#define __H3HUFF_CODEC_H__

#include "H3Common.h"

bool HuffmanDecode(uint8_t *state, uint8_t *src, size_t len, uint8_t **dst, bool last);
size_t HuffmanEncode(uint8_t *src, size_t len, uint8_t *dst, bool lower);

#endif
