#include <string.h>
#include <stdlib.h>
#include "H3Parse.h"
#include "Huffman/HuffmanCodec.h"

static int32_t H3ParsePrefixInt(H3FieldPrefixInt *st, uint32_t prefix, uint8_t **last, uint8_t *end)
{
    uint8_t   ch;
    uint32_t  mask;
    enum {
        sw_start = 0,
        sw_value
    };

    for ( ;; ) {

        if (*last == end) {
            return H3_DATA_NEEDED;
        }

        ch = **last;
        (*last)++;

        switch (st->state) {

        case sw_start:

            mask = (1 << prefix) - 1;
            st->value = ch & mask;

            if (st->value != mask) {
                goto done;
            }

            st->shift = 0;
            st->state = sw_value;
            break;

        case sw_value:

            st->value += (uint64_t) (ch & 0x7f) << st->shift;

            if (st->shift == 56
                && ((ch & 0x80) || (st->value & 0xc000000000000000)))
            {
                printf("client exceeded integer size limit\n");
                return H3_EXCESS_LOAD;
            }

            if (ch & 0x80) {
                st->shift += 7;
                break;
            }

            goto done;
        }
    }

done:

    printf("H3 parse prefix int %lu\n", st->value);

    st->state = sw_start;

    return H3_DONE;
}

static int32_t H3DecodeInsertCount(uint32_t *insertCount, H3HeaderDynamic *H3DynamicTable)
{
    uint32_t  maxEntries, fullRange, maxValue, maxWrapped, reqInsertCount;

    if (*insertCount == 0) {
        return H3_OK;
    }

    maxEntries = H3DynamicTable->capacity / 32;
    fullRange = 2 * maxEntries;

    if (*insertCount > fullRange) {
        return H3_DECOMP_FAIL;
    }

    maxValue = H3DynamicTable->base + H3DynamicTable->nelts + maxEntries;
    maxWrapped = (maxValue / fullRange) * fullRange;
    reqInsertCount = maxWrapped + *insertCount - 1;

    if (reqInsertCount > maxValue) {
        if (reqInsertCount <= fullRange) {
            return H3_DECOMP_FAIL;
        }

        reqInsertCount -= fullRange;
    }

    if (reqInsertCount == 0) {
        return H3_DECOMP_FAIL;
    }

    printf("H3 decode insertCount %u -> %u\n", *insertCount, reqInsertCount);

    *insertCount = reqInsertCount;

    return H3_OK;
}

static int32_t H3ParseFieldSectionPrefix(H3FieldSectionPrefix *st, H3HeaderDynamic *H3DynamicTable, uint8_t **last, uint8_t *end)
{
    uint8_t   ch;
    int32_t   rc;
    enum {
        sw_start = 0,
        sw_req_insert_count,
        sw_delta_base,
        sw_read_delta_base
    };

    for ( ;; ) {

        switch (st->state) {

        case sw_start:

            printf("H3 parse field section prefix\n");

            st->state = sw_req_insert_count;

            /* fall through */

        case sw_req_insert_count:

            rc = H3ParsePrefixInt(&st->pint, 8, last, end);
            if (rc != H3_DONE) {
                return rc;
            }

            st->insert_count = st->pint.value;
            st->state = sw_delta_base;

            break;

        case sw_delta_base:

            if (*last == end) {
                return H3_DATA_NEEDED;
            }

            ch = **last;
            (*last)++;

            st->sign = (ch & 0x80) ? true : false;
            st->state = sw_read_delta_base;

            /* fall through */

        case sw_read_delta_base:

            rc = H3ParsePrefixInt(&st->pint, 7, last, end);
            if (rc != H3_DONE) {
                return rc;
            }

            st->delta_base = st->pint.value;
            goto done;
        }
    }

done:

    rc = H3DecodeInsertCount(&st->insert_count, H3DynamicTable);
    if (rc != H3_OK) {
        return rc;
    }

    if (st->sign) {
        if (st->insert_count <= st->delta_base) {
            printf("client sent negative base\n");
            return H3_DECOMP_FAIL;
        }

        st->base = st->insert_count - st->delta_base - 1;

    } else {
        st->base = st->insert_count + st->delta_base;
    }

    printf("H3 parse field section prefix done "
           "insert count:%u, sign:%u, delta base:%u, base:%u\n",
           st->insert_count, st->sign, st->delta_base, st->base);

    st->state = sw_start;

    return H3_DONE;
}

static int32_t H3ParseLookup(H3HeaderDynamic *H3DynamicTable, bool dynamic, uint32_t index, H3String *name, H3String *value)
{
    uint8_t  *p;

    if (!dynamic) {
        if (H3LookupStatic(index, name, value) != H3_OK) {
            return H3_DECOMP_FAILED;
        }

        return H3_OK;
    }

    if (H3Lookup(H3DynamicTable, index, name, value) != H3_OK) {
        return H3_DECOMP_FAILED;
    }

    if (name) {
        p = (uint8_t *) malloc(name->len + 1);
        if (p == NULL) {
            return H3_ERROR;
        }

        memcpy(p, name->data, name->len);
        p[name->len] = '\0';
        name->data = p;
    }

    if (value) {
        p = (uint8_t *) malloc(value->len + 1);
        if (p == NULL) {
            return H3_ERROR;
        }

        memcpy(p, value->data, value->len);
        p[value->len] = '\0';
        value->data = p;
    }

    return H3_OK;
}

static int32_t H3ParseFieldIFL(H3HeaderDynamic *H3DynamicTable, H3Field *st, uint8_t **last, uint8_t *end)
{
    uint8_t  ch;
    int32_t  rc;
    enum {
        sw_start = 0,
        sw_index
    };

    for ( ;; ) {

        switch (st->state) {

        case sw_start:

            printf("H3 parse field LFL\n");

            if (*last == end) {
                return H3_DATA_NEEDED;
            }

            ch = **last;
            (*last)++;

            st->dynamic = (ch & 0x40) ? false : true;
            st->state = sw_index;

            /* fall through */

        case sw_index:

            rc = H3ParsePrefixInt(&st->pint, 6, last, end);
            if (rc != H3_DONE) {
                return rc;
            }

            st->index = st->pint.value;
            goto done;
        }
    }

done:

    printf("H3 parse field IFL done %s%u]", st->dynamic ? "dynamic[-" : "static[", st->index);

    if (st->dynamic) {
        st->index = st->base - st->index - 1;
    }

    rc = H3ParseLookup(H3DynamicTable, st->dynamic, st->index, &st->name, &st->value);

    if (st->dynamic) {
        st->free_name = true;
        st->free_value = true;
    }

    if (rc != H3_OK) {
        return rc;
    }

    st->state = sw_start;

    return H3_DONE;
}

static int32_t H3ParseLiteral(H3Literal *st, uint8_t **last, uint8_t *end)
{
    uint8_t   ch;
    uint32_t  n;
    enum {
        sw_start = 0,
        sw_value
    };

    for ( ;; ) {

        switch (st->state) {

        case sw_start:

            printf("H3 parse literal huff: %u, len: %u\n", st->huffman, st->length);

            n = st->length;

            if (st->huffman) {
                n = n * 8 / 5;
                st->huffstate = 0;
            }

            st->last = (uint8_t *) malloc(n + 1);
            if (st->last == NULL) {
                return H3_ERROR;
            }

            st->value.data = st->last;
            st->state = sw_value;

            /* fall through */

        case sw_value:

            if (*last == end) {
                return H3_DATA_NEEDED;
            }

            ch = **last;
            (*last)++;

            if (st->huffman) {
                if (!HuffmanDecode(&st->huffstate, &ch, 1, &st->last, st->length == 1)) {
                    return H3_ERROR;
                }

            } else {
                *st->last++ = ch;
            }

            if (--st->length) {
                break;
            }

            st->value.len = st->last - st->value.data;
            *st->last = '\0';
            goto done;
        }
    }

done:

    printf("H3 parse literal done \"%s\"\n", st->value.data);

    st->state = sw_start;

    return H3_DONE;
}

static int32_t H3ParseFieldLFLNR(H3HeaderDynamic *H3DynamicTable, H3Field *st, uint8_t **last, uint8_t *end)
{
    uint8_t  ch;
    int32_t  rc;
    enum {
        sw_start = 0,
        sw_index,
        sw_value_len,
        sw_read_value_len,
        sw_value
    };

    for ( ;; ) {

        switch (st->state) {

        case sw_start:

            printf("H3 parse field lflnr\n");

            if (*last == end) {
                return H3_DATA_NEEDED;
            }

            ch = **last;

            st->dynamic = (ch & 0x10) ? false : true;
            st->state = sw_index;

            /* fall through */

        case sw_index:

            rc = H3ParsePrefixInt(&st->pint, 4, last, end);
            if (rc != H3_DONE) {
                return rc;
            }

            st->index = st->pint.value;
            st->state = sw_value_len;
            break;

        case sw_value_len:

            if (*last == end) {
                return H3_DATA_NEEDED;
            }

            ch = **last;

            st->literal.huffman = (ch & 0x80) ? true : false;
            st->state = sw_read_value_len;

            /* fall through */

        case sw_read_value_len:

            rc = H3ParsePrefixInt(&st->pint, 7, last, end);
            if (rc != H3_DONE) {
                return rc;
            }

            st->literal.length = st->pint.value;
            if (st->literal.length == 0) {
                goto done;
            }

            st->state = sw_value;
            break;

        case sw_value:

            rc = H3ParseLiteral(&st->literal, last, end);
            if (rc != H3_DONE) {
                return rc;
            }

            st->free_value = true;
            st->value = st->literal.value;
            goto done;
        }
    }

done:

    printf("H3 parse field lflnr done %s%u] \"%s\"\n",
           st->dynamic ? "dynamic[-" : "static[", st->index, st->value.data);

    if (st->dynamic) {
        st->index = st->base - st->index - 1;
    }

    rc = H3ParseLookup(H3DynamicTable, st->dynamic, st->index, &st->name, NULL);

    if (st->dynamic) {
        st->free_name = true;
    }

    if (rc != H3_OK) {
        return rc;
    }

    st->state = sw_start;

    return H3_DONE;
}


static int32_t H3ParseFieldLFLLN(H3Field *st, uint8_t **last, uint8_t *end)
{
    uint8_t  ch;
    int32_t  rc;
    enum {
        sw_start = 0,
        sw_name_len,
        sw_name,
        sw_value_len,
        sw_read_value_len,
        sw_value
    };

    for ( ;; ) {

        switch (st->state) {

        case sw_start:

            printf("H3 parse field lflnr\n");

            if (*last == end) {
                return H3_DATA_NEEDED;
            }

            ch = **last;

            st->literal.huffman = (ch & 0x08) ? true : false;
            st->state = sw_name_len;

            /* fall through */

        case sw_name_len:

            rc = H3ParsePrefixInt(&st->pint, 3, last, end);
            if (rc != H3_DONE) {
                return rc;
            }

            st->literal.length = st->pint.value;
            if (st->literal.length == 0) {
                return H3_ERROR;
            }

            st->state = sw_name;
            break;

        case sw_name:

            rc = H3ParseLiteral(&st->literal, last, end);
            if (rc != H3_DONE) {
                return rc;
            }

            st->free_name = true;
            st->name = st->literal.value;
            st->state = sw_value_len;
            break;

        case sw_value_len:

            if (*last == end) {
                return H3_DATA_NEEDED;
            }

            ch = **last;

            st->literal.huffman = (ch & 0x80) ? true : false;
            st->state = sw_read_value_len;

            /* fall through */

        case sw_read_value_len:

            rc = H3ParsePrefixInt(&st->pint, 7, last, end);
            if (rc != H3_DONE) {
                return rc;
            }

            st->literal.length = st->pint.value;
            if (st->literal.length == 0) {
                goto done;
            }

            st->state = sw_value;
            break;

        case sw_value:

            rc = H3ParseLiteral(&st->literal, last, end);
            if (rc != H3_DONE) {
                return rc;
            }

            st->free_value = true;
            st->value = st->literal.value;
            goto done;
        }
    }

done:

    printf("H3 parse field lflln done \"%s\":\"%s\"\n", st->name.data, st->value.data);

    st->state = sw_start;

    return H3_DONE;
}


static int32_t H3ParseFieldPBI(H3HeaderDynamic *H3DynamicTable, H3Field *st, uint8_t **last, uint8_t *end)
{
    int32_t  rc;
    enum {
        sw_start = 0,
        sw_index
    };

    for ( ;; ) {

        switch (st->state) {

        case sw_start:

            printf("H3 parse field pbi\n");

            st->state = sw_index;

            /* fall through */

        case sw_index:

            rc = H3ParsePrefixInt(&st->pint, 4, last, end);
            if (rc != H3_DONE) {
                return rc;
            }

            st->index = st->pint.value;
            goto done;
        }
    }

done:

    printf("H3 parse field pbi done dynamic[+%ui]\n", st->index);

    rc = H3ParseLookup(H3DynamicTable, true, st->base + st->index, &st->name, &st->value);

    if (st->dynamic) {
        st->free_name = true;
        st->free_value = true;
    }

    if (rc != H3_OK) {
        return rc;
    }

    st->free_name = true;
    st->free_value = true;
    st->state = sw_start;

    return H3_DONE;
}


static int32_t H3ParseFieldIFLPBI(H3HeaderDynamic *H3DynamicTable, H3Field *st, uint8_t **last, uint8_t *end)
{
    uint8_t  ch;
    int32_t  rc;
    enum {
        sw_start = 0,
        sw_index,
        sw_value_len,
        sw_read_value_len,
        sw_value
    };

    for ( ;; ) {

        switch (st->state) {

        case sw_start:

            printf("H3 parse field lpbi");

            st->state = sw_index;

            /* fall through */

        case sw_index:

            rc = H3ParsePrefixInt(&st->pint, 3, last, end);
            if (rc != H3_DONE) {
                return rc;
            }

            st->index = st->pint.value;
            st->state = sw_value_len;
            break;

        case sw_value_len:

            if (*last == end) {
                return H3_DATA_NEEDED;
            }

            ch = **last;

            st->literal.huffman = (ch & 0x80) ? true : false;
            st->state = sw_read_value_len;

            /* fall through */

        case sw_read_value_len:

            rc = H3ParsePrefixInt(&st->pint, 7, last, end);
            if (rc != H3_DONE) {
                return rc;
            }

            st->literal.length = st->pint.value;
            if (st->literal.length == 0) {
                goto done;
            }

            st->state = sw_value;
            break;

        case sw_value:

            rc = H3ParseLiteral(&st->literal, last, end);
            if (rc != H3_DONE) {
                return rc;
            }

            st->free_value = true;
            st->value = st->literal.value;
            goto done;
        }
    }

done:

    printf("H3 parse field iflpbi done dynamic[+%ui] \"%s\"", st->index, st->value.data);

    rc = H3ParseLookup(H3DynamicTable, true, st->base + st->index, &st->name, NULL);

    if (st->dynamic) {
        st->free_name = true;
    }

    if (rc != H3_OK) {
        return rc;
    }

    st->state = sw_start;
    return H3_DONE;
}

static int32_t H3ParseFieldRep(H3HeaderDynamic *H3DynamicTable, H3FieldRep *st, uint32_t base, uint8_t **last, uint8_t *end)
{
    uint8_t  ch;
    int32_t  rc;
    enum {
        sw_start = 0,
        sw_field_ifl,
        sw_field_lflnr,
        sw_field_lflln,
        sw_field_pbi,
        sw_field_iflpbi
    };

    if (st->state == sw_start) {

        printf("H3 parse field representation\n");

        if (*last == end) {
            return H3_DATA_NEEDED;
        }

        ch = **last;

        memset(&st->field, 0, sizeof(H3Field));

        st->field.base = base;

        if (ch & 0x80) {
            /* Indexed Field Line */

            st->state = sw_field_ifl;

        } else if (ch & 0x40) {
            /* Literal Field Line With Name Reference */

            st->state = sw_field_lflnr;

        } else if (ch & 0x20) {
            /* Literal Field Line With Literal Name */

            st->state = sw_field_lflln;

        } else if (ch & 0x10) {
            /* Indexed Field Line With Post-Base Index */

            st->state = sw_field_pbi;

        } else {
            /* Literal Field Line With Post-Base Name Reference */

            st->state = sw_field_iflpbi;
        }
    }

    switch (st->state) {

    case sw_field_ifl:
        rc = H3ParseFieldIFL(H3DynamicTable, &st->field, last, end);
        break;

    case sw_field_lflnr:
        rc = H3ParseFieldLFLNR(H3DynamicTable, &st->field, last, end);
        break;

    case sw_field_lflln:
        rc = H3ParseFieldLFLLN(&st->field, last, end);
        break;

    case sw_field_pbi:
        rc = H3ParseFieldPBI(H3DynamicTable, &st->field, last, end);
        break;

    case sw_field_iflpbi:
        rc = H3ParseFieldIFLPBI(H3DynamicTable, &st->field, last, end);
        break;

    default:
        rc = H3_OK;
    }

    if (rc != H3_DONE) {
        return rc;
    }

    printf("H3 parse field representation done\n");

    st->state = sw_start;

    return H3_DONE;
}

int32_t H3ParseHeaders(H3Headers *hdr, H3HeaderDynamic *H3DynamicTable, uint8_t *start, uint8_t **last, uint8_t *end)
{
    int32_t   rc;
    enum {
        sw_prefix = 0,
        sw_field_rep,
        sw_done
    };

    for ( ;; ) {
        switch (hdr->state) {

        case sw_prefix:

            rc = H3ParseFieldSectionPrefix(&hdr->prefix, H3DynamicTable, last, end);

            hdr->length -= *last - start;

            if (hdr->length == 0 && rc == H3_DATA_NEEDED) {
                return H3_FRAME_ERROR;
            }

            if (rc != H3_DONE) {
                return rc;
            }

            hdr->state = sw_field_rep;

            break;

        case sw_field_rep:

            rc = H3ParseFieldRep(H3DynamicTable, &hdr->field_rep, hdr->prefix.base, last, end);

            hdr->length -= *last - start;

            if (hdr->length == 0 && rc == H3_DATA_NEEDED) {
                return H3_FRAME_ERROR;
            }

            if (rc != H3_DONE) {
                return rc;
            }

            if (hdr->length == 0) {
                goto done;
            }

            return H3_OK;
        }
    }

done:

    printf("H3 parse headers done\n");

    hdr->state = sw_prefix;

    return H3_DONE;
}
