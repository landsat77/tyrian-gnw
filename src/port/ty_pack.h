#ifndef TY_PACK_H
#define TY_PACK_H

#include <stdint.h>
#include <stdbool.h>

#define TY_PACK_MAGIC "GWTY01\0\0"

enum {
    TY_PACK_OK = 0,
    TY_PACK_ERR_MAGIC = -1,
    TY_PACK_ERR_CORRUPT = -2,
};

#ifdef __cplusplus
extern "C" {
#endif

int ty_pack_open(const uint8_t *data, uint32_t size);
bool ty_pack_find(const char *name, const uint8_t **data, uint32_t *size);
uint32_t ty_pack_count(void);
const char *ty_pack_name(uint32_t index);

#ifdef __cplusplus
}
#endif

#endif
