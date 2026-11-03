#include "crash.h"
#include <stddef.h>
#define CRASH_MAGIC UINT32_C(0x43525348)
#define CRASH_WORDS (sizeof(CrashRecord)/sizeof(uint32_t))
typedef char crash_layout_must_be_88_bytes[(sizeof(CrashRecord)==88)?1:-1];
static uint32_t checksum(const CrashRecord *r)
{
    const uint8_t *p=(const uint8_t *)r; unsigned i,b; uint32_t crc=UINT32_MAX;
    for(i=0;i<offsetof(CrashRecord,crc);i++) {
        crc^=p[i]; for(b=0;b<8;b++) crc=(crc>>1)^((crc&1)?UINT32_C(0xedb88320):0);
    }
    return ~crc;
}
bool crash_read(const volatile CrashRecord *storage,CrashRecord *out)
{
    unsigned i; const volatile uint8_t *src=(const volatile uint8_t *)storage;
    uint8_t *dst=(uint8_t *)out;
    if(storage->magic!=CRASH_MAGIC) return false;
    for(i=0;i<sizeof(CrashRecord);i++) dst[i]=src[i];
    return storage->magic==CRASH_MAGIC && out->magic==CRASH_MAGIC &&
           out->version==1 && out->crc==checksum(out);
}
void crash_write(volatile CrashRecord *storage,const CrashContext *context)
{
    CrashRecord r,old; const uint8_t *src=(const uint8_t *)&r;
    volatile uint8_t *dst=(volatile uint8_t *)storage; unsigned i;
    r.magic=CRASH_MAGIC; r.version=1;
    r.sequence=crash_read(storage,&old)?old.sequence+1:1;
    r.context=*context; r.crc=checksum(&r);
    storage->magic=0;
    for(i=sizeof(uint32_t);i<sizeof(CrashRecord);i++) dst[i]=src[i];
    storage->magic=CRASH_MAGIC; /* commit last; partial writes rejected */
}
void crash_clear(volatile CrashRecord *storage) { storage->magic=0; }
