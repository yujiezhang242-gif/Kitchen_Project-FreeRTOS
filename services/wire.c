#include "wire.h"
#include <string.h>
uint16_t wire_get16(const uint8_t *p) { return (uint16_t)(p[0] | (uint16_t)p[1]<<8); }
uint32_t wire_get32(const uint8_t *p) { return wire_get16(p) | (uint32_t)wire_get16(p+2)<<16; }
void wire_put16(uint8_t *p,uint16_t v) { p[0]=(uint8_t)v; p[1]=(uint8_t)(v>>8); }
void wire_put32(uint8_t *p,uint32_t v) { wire_put16(p,(uint16_t)v); wire_put16(p+2,(uint16_t)(v>>16)); }
uint16_t wire_crc(const uint8_t *d,size_t n)
{
    uint16_t c=0xffff; size_t i; unsigned b;
    for(i=0;i<n;i++) { c^=d[i]; for(b=0;b<8;b++) c=(uint16_t)((c>>1)^((c&1)?0xa001:0)); }
    return c;
}
size_t wire_encode(const WireFrame *f,uint8_t *o,size_t cap)
{
    size_t n;
    if(!f || !o || f->length>128) return 0;
    n=(size_t)f->length+10; if(cap<n) return 0;
    o[0]=0xaa; o[1]=0x55; o[2]=1; o[3]=f->type;
    wire_put16(o+4,f->seq); wire_put16(o+6,f->length);
    memcpy(o+8,f->data,f->length); wire_put16(o+n-2,wire_crc(o+2,n-4)); return n;
}
void wire_init(WireParser *p) { memset(p,0,sizeof(*p)); }
void wire_expire(WireParser *p,uint32_t now)
{
    if(p->used && (uint32_t)(now-p->last_ms)>=200) { p->used=0; p->timeouts++; }
}
static void discard(WireParser *p,unsigned n)
{
    p->used=(uint16_t)(p->used-n); memmove(p->buffer,p->buffer+n,p->used);
}
bool wire_feed(WireParser *p,uint8_t b,uint32_t now,WireFrame *o)
{
    uint16_t len,total;
    wire_expire(p,now); p->last_ms=now;
    if(p->used==WIRE_MAX_FRAME) discard(p,1);
    p->buffer[p->used++]=b;
    for(;;) {
        if(!p->used) return false;
        if(p->buffer[0]!=0xaa) { discard(p,1); continue; }
        if(p->used<2) return false;
        if(p->buffer[1]!=0x55) { discard(p,1); continue; }
        if(p->used<8) return false;
        len=wire_get16(p->buffer+6);
        if(p->buffer[2]!=1 || len>128) { p->bad_length++; discard(p,1); continue; }
        total=(uint16_t)(len+10); if(p->used<total) return false;
        if(wire_crc(p->buffer+2,total-4)!=wire_get16(p->buffer+total-2)) {
            p->bad_crc++; discard(p,1); continue;
        }
        o->type=p->buffer[3]; o->seq=wire_get16(p->buffer+4); o->length=len;
        memcpy(o->data,p->buffer+8,len); discard(p,total); p->frames++; return true;
    }
}
