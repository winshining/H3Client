#ifndef __H3PARSE_H__
#define __H3PARSE_H__

#include "H3Common.h"
#include "H3Table/H3Table.h"

typedef struct
{
    uint32_t                state;
    uint64_t                value;
} H3VarLenInt;

typedef struct
{
    uint32_t                state;
    uint32_t                shift;
    uint64_t                value;
} H3FieldPrefixInt;

typedef struct
{
    uint32_t                state;
    uint32_t                insert_count;
    uint32_t                delta_base;
    bool                    sign;
    uint32_t                base;
    H3FieldPrefixInt        pint;
} H3FieldSectionPrefix;

typedef struct
{
    uint32_t                state;
    uint32_t                length;
    bool                    huffman;
    H3String                value;
    uint8_t                *last;
    uint8_t                 huffstate;
} H3Literal;

typedef struct
{
    uint32_t                state;
    uint32_t                index;
    uint32_t                base;
    bool                    dynamic;

    bool                    free_name;
    bool                    free_value;

    H3String                name;
    H3String                value;

    H3FieldPrefixInt        pint;
    H3Literal               literal;
} H3Field;

typedef struct
{
    uint32_t                state;
    H3Field                 field;
} H3FieldRep;

typedef struct
{
    uint32_t                state;
    uint32_t                type;
    uint32_t                length;
    H3VarLenInt             vlint;
    H3FieldSectionPrefix    prefix;
    H3FieldRep              field_rep;
} H3Headers;

#define H3_BUFFER_MAX_LEN  4096

int32_t H3ParseHeaders(H3Headers *hdr, H3HeaderDynamic *H3DynamicTable, uint8_t *start, uint8_t **last, uint8_t *end);

#endif
