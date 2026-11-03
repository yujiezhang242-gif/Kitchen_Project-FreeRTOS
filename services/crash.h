#ifndef KITCHEN_CRASH_H
#define KITCHEN_CRASH_H
#include <stdbool.h>
#include <stdint.h>
typedef struct {
    uint32_t reason,uptime_ms,exception_return,stack[8];
    uint32_t cfsr,hfsr,mmfar,bfar,msp,psp,task_mask;
} CrashContext;
typedef struct { uint32_t magic,version,sequence; CrashContext context; uint32_t crc; } CrashRecord;
/* Storage is SRAM retained across warm resets, never a guarantee across power loss. */
bool crash_read(const volatile CrashRecord *storage,CrashRecord *out);
void crash_write(volatile CrashRecord *storage,const CrashContext *context);
void crash_clear(volatile CrashRecord *storage);
#endif
