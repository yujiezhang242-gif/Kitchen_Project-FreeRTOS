#ifndef KITCHEN_RUNTIME_H
#define KITCHEN_RUNTIME_H
#include "kitchen.h"
#include "wire.h"
#define KITCHEN_LOG_CAPACITY 16u
enum { CMD_STATUS=1,CMD_GET_CONFIG=2,CMD_SET_CONFIG=3,CMD_MANUAL=4,
       CMD_ACK=5,CMD_LOG=6,CMD_SAVE=7,CMD_DIAG=8 };
enum { RESULT_OK=0,RESULT_LENGTH=1,RESULT_VALUE=2,RESULT_BUSY=3,
       RESULT_UNSUPPORTED=4,RESULT_CONFLICT=5,RESULT_IO=6 };
typedef struct { uint32_t at_ms; uint16_t event, value; } KitchenLog;
typedef struct {
    bool used; uint32_t at_ms; WireFrame request,response;
} KitchenCached;
typedef struct {
    Kitchen control;
    KitchenSample sample;
    KitchenOutput output;
    bool manual, manual_lamp, manual_vent, dirty;
    uint32_t manual_since,manual_ms,revision,commands,duplicates,log_dropped;
    KitchenLog logs[16]; unsigned log_head,log_count;
    KitchenCached cache[4]; unsigned cache_next;
} KitchenRuntime;
void runtime_init(KitchenRuntime *r,const KitchenConfig *c);
void runtime_tick(KitchenRuntime *r,const KitchenSample *s,uint32_t now);
void runtime_log(KitchenRuntime *r,uint32_t now,uint16_t event,uint16_t value);
/* Called by the control owner only. SAVE/DIAG board extensions handled outside.
 * Manual command: [lamp 0/1, vent 0/1, duration_ms u32]; duration=0 means AUTO. */
void runtime_command(KitchenRuntime *r,const WireFrame *q,WireFrame *a,uint32_t now);
#endif
