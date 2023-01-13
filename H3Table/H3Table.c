#include <string.h>
#include <stdlib.h>
#include "H3Table.h"

H3Header  H3StaticTable[] = {
    { H3StrInit(":authority"),           H3StrInit("") },
    { H3StrInit(":path"),                H3StrInit("/") },
    { H3StrInit("age"),                  H3StrInit("0") },
    { H3StrInit("content-disposition"),  H3StrInit("") },
    { H3StrInit("content-length"),       H3StrInit("0") },
    { H3StrInit("cookie"),               H3StrInit("") },
    { H3StrInit("date"),                 H3StrInit("") },
    { H3StrInit("etag"),                 H3StrInit("") },
    { H3StrInit("if-modified-since"),    H3StrInit("") },
    { H3StrInit("if-none-match"),        H3StrInit("") },
    { H3StrInit("last-modified"),        H3StrInit("") },
    { H3StrInit("link"),                 H3StrInit("") },
    { H3StrInit("location"),             H3StrInit("") },
    { H3StrInit("referer"),              H3StrInit("") },
    { H3StrInit("set-cookie"),           H3StrInit("") },
    { H3StrInit(":method"),              H3StrInit("CONNECT") },
    { H3StrInit(":method"),              H3StrInit("DELETE") },
    { H3StrInit(":method"),              H3StrInit("GET") },
    { H3StrInit(":method"),              H3StrInit("HEAD") },
    { H3StrInit(":method"),              H3StrInit("OPTIONS") },
    { H3StrInit(":method"),              H3StrInit("POST") },
    { H3StrInit(":method"),              H3StrInit("PUT") },
    { H3StrInit(":scheme"),              H3StrInit("http") },
    { H3StrInit(":scheme"),              H3StrInit("https") },
    { H3StrInit(":status"),              H3StrInit("103") },
    { H3StrInit(":status"),              H3StrInit("200") },
    { H3StrInit(":status"),              H3StrInit("304") },
    { H3StrInit(":status"),              H3StrInit("404") },
    { H3StrInit(":status"),              H3StrInit("503") },
    { H3StrInit("accept"),               H3StrInit("*/*") },
    { H3StrInit("accept"),               H3StrInit("application/dns-message") },
    { H3StrInit("accept-encoding"),      H3StrInit("gzip, deflate, br") },
    { H3StrInit("accept-ranges"),        H3StrInit("bytes") },
    { H3StrInit("access-control-allow-headers"),
                                         H3StrInit("cache-control") },
    { H3StrInit("access-control-allow-headers"),
                                         H3StrInit("content-type") },
    { H3StrInit("access-control-allow-origin"),
                                         H3StrInit("*") },
    { H3StrInit("cache-control"),        H3StrInit("max-age=0") },
    { H3StrInit("cache-control"),        H3StrInit("max-age=2592000") },
    { H3StrInit("cache-control"),        H3StrInit("max-age=604800") },
    { H3StrInit("cache-control"),        H3StrInit("no-cache") },
    { H3StrInit("cache-control"),        H3StrInit("no-store") },
    { H3StrInit("cache-control"),
                                        H3StrInit("public, max-age=31536000") },
    { H3StrInit("content-encoding"),     H3StrInit("br") },
    { H3StrInit("content-encoding"),     H3StrInit("gzip") },
    { H3StrInit("content-type"),         H3StrInit("application/dns-message") },
    { H3StrInit("content-type"),         H3StrInit("application/javascript") },
    { H3StrInit("content-type"),         H3StrInit("application/json") },
    { H3StrInit("content-type"),
                               H3StrInit("application/x-www-form-urlencoded") },
    { H3StrInit("content-type"),         H3StrInit("image/gif") },
    { H3StrInit("content-type"),         H3StrInit("image/jpeg") },
    { H3StrInit("content-type"),         H3StrInit("image/png") },
    { H3StrInit("content-type"),         H3StrInit("text/css") },
    { H3StrInit("content-type"),         H3StrInit("text/html;charset=utf-8") },
    { H3StrInit("content-type"),         H3StrInit("text/plain") },
    { H3StrInit("content-type"),
                                        H3StrInit("text/plain;charset=utf-8") },
    { H3StrInit("range"),                H3StrInit("bytes=0-") },
    { H3StrInit("strict-transport-security"),
                                         H3StrInit("max-age=31536000") },
    { H3StrInit("strict-transport-security"),
                              H3StrInit("max-age=31536000;includesubdomains") },
    { H3StrInit("strict-transport-security"),
                      H3StrInit("max-age=31536000;includesubdomains;preload") },
    { H3StrInit("vary"),                 H3StrInit("accept-encoding") },
    { H3StrInit("vary"),                 H3StrInit("origin") },
    { H3StrInit("x-content-type-options"),
                                         H3StrInit("nosniff") },
    { H3StrInit("x-xss-protection"),     H3StrInit("1;mode=block") },
    { H3StrInit(":status"),              H3StrInit("100") },
    { H3StrInit(":status"),              H3StrInit("204") },
    { H3StrInit(":status"),              H3StrInit("206") },
    { H3StrInit(":status"),              H3StrInit("302") },
    { H3StrInit(":status"),              H3StrInit("400") },
    { H3StrInit(":status"),              H3StrInit("403") },
    { H3StrInit(":status"),              H3StrInit("421") },
    { H3StrInit(":status"),              H3StrInit("425") },
    { H3StrInit(":status"),              H3StrInit("500") },
    { H3StrInit("accept-language"),      H3StrInit("") },
    { H3StrInit("access-control-allow-credentials"),
                                         H3StrInit("FALSE") },
    { H3StrInit("access-control-allow-credentials"),
                                         H3StrInit("TRUE") },
    { H3StrInit("access-control-allow-headers"),
                                         H3StrInit("*") },
    { H3StrInit("access-control-allow-methods"),
                                         H3StrInit("get") },
    { H3StrInit("access-control-allow-methods"),
                                         H3StrInit("get, post, options") },
    { H3StrInit("access-control-allow-methods"),
                                         H3StrInit("options") },
    { H3StrInit("access-control-expose-headers"),
                                         H3StrInit("content-length") },
    { H3StrInit("access-control-request-headers"),
                                         H3StrInit("content-type") },
    { H3StrInit("access-control-request-method"),
                                         H3StrInit("get") },
    { H3StrInit("access-control-request-method"),
                                         H3StrInit("post") },
    { H3StrInit("alt-svc"),              H3StrInit("clear") },
    { H3StrInit("authorization"),        H3StrInit("") },
    { H3StrInit("content-security-policy"),
             H3StrInit("script-src 'none';object-src 'none';base-uri 'none'") },
    { H3StrInit("early-data"),           H3StrInit("1") },
    { H3StrInit("expect-ct"),            H3StrInit("") },
    { H3StrInit("forwarded"),            H3StrInit("") },
    { H3StrInit("if-range"),             H3StrInit("") },
    { H3StrInit("origin"),               H3StrInit("") },
    { H3StrInit("purpose"),              H3StrInit("prefetch") },
    { H3StrInit("server"),               H3StrInit("") },
    { H3StrInit("timing-allow-origin"),  H3StrInit("*") },
    { H3StrInit("upgrade-insecure-requests"),
                                         H3StrInit("1") },
    { H3StrInit("user-agent"),           H3StrInit("") },
    { H3StrInit("x-forwarded-for"),      H3StrInit("") },
    { H3StrInit("x-frame-options"),      H3StrInit("deny") },
    { H3StrInit("x-frame-options"),      H3StrInit("sameorigin") }
};

H3HeaderDynamic  H3DynamicTable;

int32_t H3RefInsert(H3HeaderDynamic *H3DynamicTable, bool dynamic, uint32_t index, H3String *value)
{
    H3String  name;

    if (dynamic) {
        printf("H3 ref insert dynamic[%u] \"%s\"\n", index, value->data);

        if (H3DynamicTable->base + H3DynamicTable->nelts <= index) {
            return H3_ENCSTREAM_ERROR;
        }

        index = H3DynamicTable->base + H3DynamicTable->nelts - 1 - index;

        if (H3Lookup(H3DynamicTable, index, &name, NULL) != H3_OK) {
            return H3_ENCSTREAM_ERROR;
        }

    } else {
        printf("H3 ref insert static[%u] \"%s\"\n", index, value->data);

        if (H3LookupStatic(index, &name, NULL) != H3_OK) {
            return H3_ENCSTREAM_ERROR;
        }
    }

    return H3Insert(H3DynamicTable, &name, value);
}

int32_t H3Insert(H3HeaderDynamic *H3DynamicTable, H3String *name, H3String *value)
{
    uint8_t   *p;
    size_t     size;
    H3Header  *field;

    size = name->len + value->len + 32;

    if (size > H3DynamicTable->capacity) {
        printf("H3 not enough dynamic table capacity\n");
        return H3_ENCSTREAM_ERROR;
    }

    printf("H3 insert [%u] \"%s\":\"%s\", size:%ld\n",
           H3DynamicTable->base + H3DynamicTable->nelts, name->data, value->data, size);

    p = (uint8_t *) malloc(sizeof(H3Header) + name->len + value->len);
    if (p == NULL) {
        return H3_ERROR;
    }

    field = (H3Header *) p;

    field->Name.data = p + sizeof(H3Header);
    field->Name.len = name->len;
    memcpy(field->Name.data, name->data, name->len);
    field->Value.len = value->len;
    field->Value.data = field->Name.data + field->Name.len;
    memcpy(field->Value.data, value->data, value->len);

    H3DynamicTable->elts[H3DynamicTable->nelts++] = field;
    H3DynamicTable->size += size;
    H3DynamicTable->insert_count++;

	return H3_OK;
}

int32_t H3LookupStatic(uint32_t index, H3String *name, H3String *value)
{
    uint32_t         nelts;
    const H3Header  *field;

    nelts = sizeof(H3StaticTable) / sizeof(H3StaticTable[0]);

    if (index >= nelts) {
        printf("H3 static[%u] lookup out of bounds: %u\n", index, nelts);
        return H3_ERROR;
    }

    field = &H3StaticTable[index];

    printf("H3 static[%u] lookup \"%s\":\"%s\"\n", index, field->Name.data, field->Value.data);

    if (name) {
        *name = field->Name;
    }

    if (value) {
        *value = field->Value;
    }

    return H3_OK;
}

int32_t H3Lookup(H3HeaderDynamic *H3DynamicTable, uint32_t index, H3String *name, H3String *value)
{
    H3Header  *field;

    if (index < H3DynamicTable->base || index - H3DynamicTable->base >= H3DynamicTable->nelts) {
        printf("H3 dynamic[%u] lookup out of bounds: [%u,%u]\n",
               index, H3DynamicTable->base, H3DynamicTable->base + H3DynamicTable->nelts);
        return H3_ERROR;
    }

    field = H3DynamicTable->elts[index - H3DynamicTable->base];

    printf("H3 dynamic[%u] lookup \"%s\":\"%s\"\n", index, field->Name.data, field->Value.data);

    if (name) {
        *name = field->Name;
    }

    if (value) {
        *value = field->Value;
    }

    return H3_OK;
}
