#include "runtime.h"
#include "health.h"
#include "dht_decoder.h"
#include "timing.h"
#include "crash.h"
#include "text.h"
#include <limits.h>
#include "config_store.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
typedef struct { uint8_t bytes[2][1024]; int budget,fail_read_slot,erases; } Flash;
static bool read_mem(void *v,unsigned s,unsigned o,uint8_t *p,size_t n)
{ Flash *f=v; assert(s<2 && o+n<=1024); if((int)s==f->fail_read_slot) return false; memcpy(p,f->bytes[s]+o,n); return true; }
static bool erase_mem(void *v,unsigned s)
{ Flash *f=v; if(f->budget==0) return false; if(f->budget>0) f->budget--; memset(f->bytes[s],255,1024); f->erases++; return true; }
static bool program_mem(void *v,unsigned s,unsigned o,const uint8_t *p,size_t n)
{
    Flash *f=v; size_t i; assert(n%2==0 && o%2==0 && o+n<=1024);
    for(i=0;i<n;i+=2) {
        if(f->budget==0) return false;
        if(f->budget>0) f->budget--;
        assert((f->bytes[s][o+i]&p[i])==p[i]); assert((f->bytes[s][o+i+1]&p[i+1])==p[i+1]);
        f->bytes[s][o+i]&=p[i]; f->bytes[s][o+i+1]&=p[i+1];
    } return true;
}
static void storage_test(void)
{
    Flash f, baseline; ConfigStore s,reboot; KitchenConfig c,d; int cut;
    ConfigBackend io={&f,read_mem,erase_mem,program_mem};
    memset(&f,255,sizeof f); f.budget=-1; f.erases=0;
    assert(!config_load(&s,&io,&c)); assert(config_save(&s,&c)); baseline=f;
    c.temp_on=350;
    /* Interrupt before each erase/halfword, including the final commit word. */
    for(cut=0;cut<=20;cut++) {
        f=baseline; f.budget=-1; assert(config_load(&s,&io,&d)); f.budget=cut;
        (void)config_save(&s,&c); f.budget=-1;
        assert(config_load(&reboot,&io,&d));
        assert(d.temp_on==(cut<20?300:350));
    }
    f=baseline; f.budget=-1; assert(config_load(&s,&io,&d)); assert(config_save(&s,&c));
    f.bytes[s.active][(s.next[s.active]-1)*40+20]^=1; assert(config_load(&reboot,&io,&d)); assert(d.temp_on==300);
    /* Fill active page; power failure during rollover keeps previous page valid. */
    f=baseline; f.budget=-1; assert(config_load(&s,&io,&d));
    { unsigned i; for(i=1;i<CONFIG_RECORDS_PER_PAGE;i++) { c.temp_on=(int16_t)(300+i); assert(config_save(&s,&c)); } }
    baseline=f; c.temp_on=400;
    for(cut=0;cut<=21;cut++) {
        f=baseline; f.budget=-1; assert(config_load(&s,&io,&d)); f.budget=cut;
        (void)config_save(&s,&c); f.budget=-1;
        assert(config_load(&reboot,&io,&d)); assert(d.temp_on==(cut<21?324:400));
    }
    /* Retry after a torn append must not rewrite programmed bits. */
    f=baseline; f.budget=-1; assert(config_load(&s,&io,&d));
    f.budget=5; assert(!config_save(&s,&c)); f.budget=-1;
    assert(config_save(&s,&c)); assert(config_load(&reboot,&io,&d)); assert(d.temp_on==400);
    /* A read error must not cause erase of a potentially valid page. */
    f.fail_read_slot=1; assert(config_load(&s,&io,&d)); assert(s.io_error);
    { int count=f.erases; assert(!config_save(&s,&c)); assert(f.erases==count); }
    f.fail_read_slot=-1; assert(config_load(&s,&io,&d));
    /* Generation wraps from UINT32_MAX to zero without selecting old data. */
    memset(&f,255,sizeof f); f.budget=-1; f.erases=0;
    assert(!config_load(&s,&io,&c)); assert(config_save(&s,&c));
    wire_put32(f.bytes[0]+8,UINT32_MAX); wire_put16(f.bytes[0]+36,wire_crc(f.bytes[0],36));
    assert(config_load(&s,&io,&d) && s.generation==UINT32_MAX);
    c.temp_on=351; assert(config_save(&s,&c)); assert(config_load(&reboot,&io,&d));
    assert(reboot.generation==0 && d.temp_on==351);
    /* 100 writes use page append slots rather than 100 erases. */
    memset(&f,255,sizeof f); f.budget=-1; f.erases=0;
    assert(!config_load(&s,&io,&c));
    { unsigned i; for(i=0;i<100;i++) { c.temp_on=(int16_t)(300+i); assert(config_save(&s,&c)); } }
    assert(f.erases==3); assert(config_load(&reboot,&io,&d) && d.temp_on==399);
    puts("PASS: journal power cuts/rollover/retry/CRC, read-error protection, generation wrap, 100 writes / 3 erases");
}
static void protocol_test(void)
{
    WireParser p; WireFrame q={0},a; uint8_t b[138]; size_t n,i; unsigned got=0;
    q.type=3; q.seq=0x1234; q.length=128;
    for(i=0;i<128;i++) q.data[i]=(uint8_t)i;
    n=wire_encode(&q,b,sizeof b); assert(n==138); wire_init(&p);
    assert(wire_crc((const uint8_t *)"123456789",9)==0x4b37);
    for(i=0;i<n;i++) if(wire_feed(&p,b[i],1,&a)) got++;
    assert(got==1 && a.seq==q.seq && !memcmp(a.data,q.data,128));
    b[20]^=1; for(i=0;i<n;i++) assert(!wire_feed(&p,b[i],2,&a)); assert(p.bad_crc==1);
    b[20]^=1;
    for(i=0;i<5;i++) (void)wire_feed(&p,b[i],3,&a);
    wire_expire(&p,203); assert(p.used==0 && p.timeouts==1);
    for(i=0;i<n;i++) if(wire_feed(&p,b[i],204,&a)) got++;
    assert(got==2);
    for(i=0;i<8;i++) (void)wire_feed(&p,b[i],205,&a);
    /* Abandon truncated frame by timeout; next packet accepted. */
    wire_expire(&p,405);
    b[6]=129; for(i=0;i<8;i++) (void)wire_feed(&p,b[i],406,&a); assert(p.bad_length>0);
    q.length=0; n=wire_encode(&q,b,sizeof b);
    for(i=0;i<n;i++) if(wire_feed(&p,b[i],407,&a)) got++;
    assert(got==3);
    /* Deterministic random stream exercises bounds and resynchronization. */
    { uint32_t x=7; for(i=0;i<100000;i++) { x=x*1664525u+1013904223u; (void)wire_feed(&p,(uint8_t)(x>>24),(uint32_t)i,&a); } }
    wire_expire(&p,200000);
    for(i=0;i<n;i++) if(wire_feed(&p,b[i],200001,&a)) got++;
    assert(got==4); puts("PASS: framing, max length, CRC, timeout, malformed length, random stream recovery");
}
static void runtime_test(void)
{
    KitchenRuntime r; KitchenConfig c=kitchen_defaults(); WireFrame q={0},a;
    KitchenSample s={0,0,250,500,1000,true,true}; unsigned i;
    runtime_init(&r,&c); runtime_tick(&r,&s,0);
    q.type=CMD_MANUAL; q.seq=1; q.length=6; q.data[0]=1; q.data[1]=0; wire_put32(q.data+2,1000);
    runtime_command(&r,&q,&a,10); assert(a.data[0]==0 && r.manual && r.output.lamp);
    runtime_command(&r,&q,&a,999); assert(r.duplicates==1 && r.manual_since==10);
    q.data[0]=0; runtime_command(&r,&q,&a,1000); assert(a.data[0]==RESULT_CONFLICT);
    runtime_tick(&r,NULL,1010); assert(!r.manual && !r.output.lamp);
    s.temp=350; s.climate_ms=1011; s.light_ms=1011; runtime_tick(&r,&s,1011);
    q.seq=2; q.data[0]=0; runtime_command(&r,&q,&a,1012); assert(r.output.ventilation && r.output.buzzer);
    q.seq=3; q.type=CMD_ACK; q.length=0; runtime_command(&r,&q,&a,1013); assert(!r.output.buzzer && r.output.hot);
    s.climate_valid=false; runtime_tick(&r,&s,1014); assert(r.output.buzzer);
    q.seq=4; q.type=CMD_SET_CONFIG; q.length=20; c.temp_on=360; config_pack(&c,q.data);
    runtime_command(&r,&q,&a,1015); assert(a.data[0]==0 && r.dirty && r.revision==1);
    runtime_command(&r,&q,&a,1016); assert(r.revision==1);
    q.seq=5; runtime_command(&r,&q,&a,1016); assert(r.revision==1);
    for(i=0;i<30;i++) runtime_log(&r,i,10,(uint16_t)i);
    q.type=CMD_LOG; q.length=1; q.data[0]=0; runtime_command(&r,&q,&a,1017);
    assert(a.data[0]==0 && wire_get16(a.data+8)==29 && r.log_count==16 && r.log_dropped>0);
    puts("PASS: manual expiry, fault precedence, dedup, conflict, config revision, log ring");
}
static void health_test(void)
{
    KitchenHealth h; uint32_t last[5]={0,0,0,0,0}; unsigned i;
    health_init(&h,0); assert(health_stale(&h,last,2999)==0);
    assert(health_stale(&h,last,3000)==31);
    for(i=0;i<5;i++) last[i]=3000;
    assert(health_stale(&h,last,4500)==0);
    last[2]=0; assert(health_stale(&h,last,4500)==4);
    health_init(&h,UINT32_MAX-4000);
    for(i=0;i<5;i++) last[i]=UINT32_MAX-500;
    assert(health_stale(&h,last,500)==0);
    assert(health_stale(&h,last,1500)==31);
    puts("PASS: watchdog worker deadlines, grace period, stalled worker, clock wraparound");
}
static void dht_frame(volatile DhtDecoder *d,const uint8_t bytes[5],uint16_t start)
{
    unsigned bit; uint16_t t=start;
    dht_decoder_start(d); dht_decoder_edge(d,true,t);
    t=(uint16_t)(t+20); dht_decoder_edge(d,false,t);
    t=(uint16_t)(t+80); dht_decoder_edge(d,true,t);
    t=(uint16_t)(t+80); dht_decoder_edge(d,false,t);
    for(bit=0;bit<40;bit++) {
        t=(uint16_t)(t+50); dht_decoder_edge(d,true,t);
        t=(uint16_t)(t+((bytes[bit/8]&(0x80u>>(bit%8)))?70:27));
        dht_decoder_edge(d,false,t);
    }
}
static void dht_test(void)
{
    volatile DhtDecoder d; uint8_t bytes[5]={50,0,25,0,75}; int16_t temp; uint16_t humid;
    dht_frame(&d,bytes,1000); assert(dht_decoder_result(&d,&temp,&humid));
    assert(temp==250 && humid==500);
    dht_frame(&d,bytes,65000); assert(dht_decoder_result(&d,&temp,&humid));
    bytes[4]^=1; dht_frame(&d,bytes,1000); assert(!dht_decoder_result(&d,&temp,&humid));
    dht_decoder_start(&d); dht_decoder_edge(&d,false,0); dht_decoder_edge(&d,true,5);
    assert(d.failed && !dht_decoder_result(&d,&temp,&humid));
    dht_decoder_start(&d); assert(!dht_decoder_result(&d,&temp,&humid));
    puts("PASS: DHT edge decoder, timer wrap, checksum, glitch and incomplete frame");
}
static void independent_fault_test(void)
{
    Kitchen k; KitchenConfig c=kitchen_defaults(); KitchenOutput o;
    KitchenSample s={0,0,250,500,3000,false,true};
    assert(kitchen_init(&k,&c)); o=kitchen_step(&k,&s,0);
    assert(o.climate_fault && !o.light_fault && o.lamp && o.ventilation);
    kitchen_ack(&k,0); o=kitchen_step(&k,&s,1); assert(!o.buzzer);
    s.light_valid=false; o=kitchen_step(&k,&s,2); assert(o.buzzer && o.light_fault);
    s.climate_valid=true; o=kitchen_step(&k,&s,3);
    assert(!o.climate_fault && o.light_fault && !o.ventilation && o.lamp);
    s.light_valid=true; s.climate_ms=5999; s.light_ms=0;
    o=kitchen_step(&k,&s,6000); assert(!o.climate_fault && o.light_fault);
    s.climate_ms=0; s.light_ms=5999; s.light=1000;
    o=kitchen_step(&k,&s,6001); assert(o.climate_fault && !o.light_fault && !o.lamp);
    puts("PASS: independent sensor faults, independent timestamps, healthy-channel continuity, new-source re-alarm");
}
static void crash_test(void)
{
    volatile CrashRecord retained={0}; CrashRecord out; CrashContext context={0};
    assert(!crash_read(&retained,&out));
    context.reason=41; context.stack[6]=0x08001235; context.cfsr=0x100;
    crash_write(&retained,&context); assert(crash_read(&retained,&out));
    assert(out.sequence==1 && out.context.stack[6]==0x08001235 && out.context.reason==41);
    context.task_mask=4; crash_write(&retained,&context); assert(crash_read(&retained,&out) && out.sequence==2);
    retained.context.uptime_ms^=1; assert(!crash_read(&retained,&out));
    crash_write(&retained,&context); assert(crash_read(&retained,&out));
    retained.magic=0; assert(!crash_read(&retained,&out));
    crash_write(&retained,&context); crash_clear(&retained); assert(!crash_read(&retained,&out));
    puts("PASS: retained crash record, register payload, sequence, corruption, torn commit, clear");
}
static void engineering_test(void)
{
    TimingStats timing; char buf[8]; char full[80];
    timing_init(&timing,5,25); timing_start(&timing,UINT32_MAX-10);
    timing_finish(&timing,UINT32_MAX-10,UINT32_MAX-7);
    timing_start(&timing,9); assert(timing.max_interval==20 && timing.late_intervals==0);
    timing_finish(&timing,9,15); assert(timing.overruns==1 && timing.max_execution==6);
    timing_start(&timing,40); assert(timing.late_intervals==1);
    assert(text_format(buf,sizeof buf,"%s:%u","ABC",123456u)==10);
    assert(strcmp(buf,"ABC:123")==0);
    assert(text_format(NULL,0,"%d",INT_MIN)==11);
    (void)text_format(full,sizeof full,"%d %u %lu %%",INT_MIN,UINT_MAX,ULONG_MAX);
    assert(strstr(full,"-2147483648")!=NULL);
    puts("PASS: hardware-timer interval accounting, overruns, wraparound, bounded no-heap formatter");
}
int main(void) { protocol_test(); storage_test(); runtime_test(); health_test(); dht_test(); independent_fault_test(); crash_test(); engineering_test(); return 0; }
