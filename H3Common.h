#ifndef __H3COMMON_H__
#define __H3COMMON_H__

#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#define H3_OK              0
#define H3_ERROR           -1
#define H3_DONE            -2
#define H3_DATA_NEEDED     -3
#define H3_EXCESS_LOAD     -4
#define H3_DECOMP_FAIL     -5
#define H3_FRAME_ERROR     -6
#define H3_DECOMP_FAILED   -7
#define H3_ENCSTREAM_ERROR -8

typedef struct
{
    size_t   len;
    uint8_t *data;
} H3String;

#define H3StrInit(str)     { sizeof(str) - 1, (uint8_t *) str }

#endif
