#include "config_store.h"
#include "wire.h"
#include <string.h>
void config_pack(const KitchenConfig *c,uint8_t p[CONFIG_BYTES])
{
    wire_put16(p,(uint16_t)c->temp_on); wire_put16(p+2,(uint16_t)c->temp_off);
    wire_put16(p+4,c->humid_on); wire_put16(p+6,c->humid_off);
    wire_put16(p+8,c->dark_on); wire_put16(p+10,c->dark_off);
    wire_put32(p+12,c->stale_ms); wire_put32(p+16,c->mute_ms);
}
static int16_t signed16(uint16_t x)
{ return x<=32767 ? (int16_t)x : (int16_t)((int32_t)x-65536); }
bool config_unpack(KitchenConfig *c,const uint8_t *p,size_t len)
{
    Kitchen check; KitchenConfig v;
    if(len!=CONFIG_BYTES) return false;
    v.temp_on=signed16(wire_get16(p)); v.temp_off=signed16(wire_get16(p+2));
    v.humid_on=wire_get16(p+4); v.humid_off=wire_get16(p+6);
    v.dark_on=wire_get16(p+8); v.dark_off=wire_get16(p+10);
    v.stale_ms=wire_get32(p+12); v.mute_ms=wire_get32(p+16);
    if(!kitchen_init(&check,&v)) return false;
    *c=v; return true;
}
static bool record(const uint8_t *r,KitchenConfig *c)
{
    return wire_get32(r)==UINT32_C(0x4b495443) && wire_get16(r+4)==1 &&
        wire_get16(r+6)==CONFIG_BYTES && wire_get16(r+36)==wire_crc(r,36) &&
        wire_get16(r+38)==0x5aa5 && config_unpack(c,r+16,20);
}
static bool erased(const uint8_t *r)
{
    unsigned i;
    for(i=0;i<CONFIG_RECORD_BYTES;i++) if(r[i]!=0xff) return false;
    return true;
}
bool config_load(ConfigStore *s,const ConfigBackend *io,KitchenConfig *c)
{
    uint8_t r[40]; KitchenConfig candidate; unsigned page,index; bool found=false;
    s->io=*io; s->active=-1; s->generation=0;
    s->next[0]=s->next[1]=0; s->io_error=false; s->needs_reload=false;
    for(page=0;page<2;page++) for(index=0;index<CONFIG_RECORDS_PER_PAGE;index++) {
        uint32_t gen;
        if(!io->read(io->context,page,index*40,r,sizeof r)) { s->io_error=true; continue; }
        /* Never overwrite a torn record: only completely erased slots are free.
         * Advance past all occupied slots even if their CRC/commit is invalid. */
        if(!erased(r)) s->next[page]=index+1;
        if(!record(r,&candidate)) continue;
        gen=wire_get32(r+8);
        if(!found || (gen!=s->generation && (uint32_t)(gen-s->generation)<UINT32_C(0x80000000))) {
            *c=candidate; s->active=(int)page; s->generation=gen; found=true;
        }
    }
    if(!found) *c=kitchen_defaults();
    s->needs_reload=s->io_error;
    return found;
}
bool config_save(ConfigStore *s,const KitchenConfig *c)
{
    uint8_t r[40],verify[40]; Kitchen check; KitchenConfig decoded;
    unsigned target,offset;
    if(!kitchen_init(&check,c)) return false;
    if(s->needs_reload) {
        ConfigBackend backend=s->io;
        (void)config_load(s,&backend,&decoded);
    }
    /* An unread page might contain the only committed copy; do not erase it. */
    if(s->io_error) return false;
    target=s->active<0?0u:(unsigned)s->active;
    if(s->next[target]>=CONFIG_RECORDS_PER_PAGE) {
        target=s->active==0?1u:0u;
        if(!s->io.erase(s->io.context,target)) { s->needs_reload=true; return false; }
        s->next[target]=0;
    }
    offset=s->next[target]*40;
    memset(r,0xff,sizeof r); wire_put32(r,UINT32_C(0x4b495443));
    wire_put16(r+4,1); wire_put16(r+6,20); wire_put32(r+8,s->generation+1);
    config_pack(c,r+16); wire_put16(r+36,wire_crc(r,36)); wire_put16(r+38,0x5aa5);
    s->next[target]++; /* Reserve even when programming later fails. */
    if(!s->io.program(s->io.context,target,offset,r,38) ||
       !s->io.read(s->io.context,target,offset,verify,38) || memcmp(r,verify,38)!=0 ||
       !s->io.program(s->io.context,target,offset+38,r+38,2) ||
       !s->io.read(s->io.context,target,offset,verify,40) || !record(verify,&decoded)) {
        s->needs_reload=true; return false;
    }
    s->active=(int)target; s->generation++; return true;
}
