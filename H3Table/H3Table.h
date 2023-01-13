#ifndef __H3TABLE_H__
#define __H3TABLE_H__

#include "H3Common.h"

typedef struct
{
    H3String  Name;
    H3String  Value;
} H3Header;

typedef struct {
    H3Header         **elts;
    uint32_t           nelts;
    uint32_t           base;
    size_t             size;
    size_t             capacity;
    uint64_t           insert_count;
} H3HeaderDynamic;

extern H3Header H3StaticTable[];

extern H3HeaderDynamic  H3DynamicTable;

int32_t H3RefInsert(H3HeaderDynamic *H3DynamicTable, bool dynamic, uint32_t index, H3String *value);
int32_t H3Insert(H3HeaderDynamic *H3DynamicTable, H3String *name, H3String *value);
int32_t H3LookupStatic(uint32_t index, H3String *name, H3String *value);
int32_t H3Lookup(H3HeaderDynamic *H3DynamicTable, uint32_t index, H3String *name, H3String *value);

#endif
