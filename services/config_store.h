#ifndef KITCHEN_STORE_H
#define KITCHEN_STORE_H
#include "kitchen.h"
#include <stddef.h>
#define CONFIG_BYTES 20u
#define CONFIG_RECORD_BYTES 40u
#define CONFIG_PAGE_BYTES 1024u
#define CONFIG_RECORDS_PER_PAGE (CONFIG_PAGE_BYTES / CONFIG_RECORD_BYTES)
/* Backend operations must be synchronous, serialized, and return false on failure.
 * Two physically independent erase units; program writes only 1->0. */
typedef struct {
    void *context;
    bool (*read)(void *,unsigned slot,unsigned offset,uint8_t *,size_t);
    bool (*erase)(void *,unsigned slot);
    bool (*program)(void *,unsigned slot,unsigned offset,const uint8_t *,size_t);
} ConfigBackend;
typedef struct {
    ConfigBackend io;
    int active;
    unsigned next[2];
    uint32_t generation;
    bool io_error, needs_reload;
} ConfigStore;
void config_pack(const KitchenConfig *c,uint8_t out[CONFIG_BYTES]);
bool config_unpack(KitchenConfig *c,const uint8_t *in,size_t len);
bool config_load(ConfigStore *s,const ConfigBackend *io,KitchenConfig *c);
bool config_save(ConfigStore *s,const KitchenConfig *c);
#endif
