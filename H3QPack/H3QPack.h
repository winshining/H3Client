#ifndef __H3QPACK_H__
#define __H3QPACK_H__

#include "H3Common.h"

#define H3_ENCODER_INSTRUCTION_CAP   0x20
#define H3_ENCODER_INSTRUCTION_INR   0x80
#define H3_ENCODER_INSTRUCTION_ILN   0x40
#define H3_ENCODER_INSTRUCTION_DUP   0x00

#define H3_CODEC_INSERT_NAME_UNKNOWN 0x00
#define H3_CODEC_INSERT_NAME_HUFFMAN 0x20
#define H3_CODEC_INSERT_VAL_UNKNOWN  0x00
#define H3_CODEC_INSERT_VAL_HUFFMAN  0x80

uint8_t *H3EncodeVarlenInt(uint8_t *p, uint64_t value, uint64_t *bufLen);
uint8_t *H3EncodePrefixInt(uint8_t *p, uint64_t value, uint32_t prefix, uint64_t *bufLen);
uint8_t *H3EncodeInsertNR(uint8_t *p, uint32_t index, uint8_t *data, size_t len, uint64_t *bufLen);
uint8_t *H3EncodeInsertLN(uint8_t *p, uint8_t *name, size_t nameLen, uint8_t *val, size_t valLen, uint64_t *bufLen);
uint8_t *H3EncodeFieldSectionPrefix(uint8_t *p, uint32_t insertCount, bool sign, uint32_t deltaBase, uint64_t *bufLen);
uint8_t *H3EncodeFieldIFL(uint8_t *p, bool dynamic, uint32_t index, uint64_t *bufLen);
uint8_t *H3EncodeFieldLFLNR(uint8_t *p, bool dynamic, uint32_t index, uint8_t *data, size_t len, uint64_t *bufLen);
uint8_t *H3EncodeFieldLFLLN(uint8_t *p, H3String *name, H3String *value, uint64_t *bufLen);
uint8_t *H3EncodeFieldIFLPBI(uint8_t *p, uint32_t index, uint64_t *bufLen);
uint8_t *H3EncodeFieldlfLPBNR(uint8_t *p, uint32_t index, uint8_t *data, size_t len, uint64_t *bufLen);

#endif
