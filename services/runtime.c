#include "runtime.h"
#include "config_store.h"
#include <string.h>
static uint16_t flags(KitchenOutput o)
{
    return (uint16_t)((o.lamp?1:0)|(o.ventilation?2:0)|(o.buzzer?4:0)|
                      (o.hot?8:0)|(o.sensor_fault?16:0)|(o.climate_fault?32:0)|(o.light_fault?64:0));
}
void runtime_init(KitchenRuntime *r,const KitchenConfig *c)
{
    KitchenConfig fallback;
    memset(r,0,sizeof(*r));
    if(!kitchen_init(&r->control,c)) { fallback=kitchen_defaults(); (void)kitchen_init(&r->control,&fallback); }
}
void runtime_log(KitchenRuntime *r,uint32_t now,uint16_t event,uint16_t value)
{
    KitchenLog *l=&r->logs[r->log_head];
    l->at_ms=now; l->event=event; l->value=value;
    r->log_head=(r->log_head+1)%16;
    if(r->log_count<16) r->log_count++; else r->log_dropped++;
}
void runtime_tick(KitchenRuntime *r,const KitchenSample *s,uint32_t now)
{
    uint16_t before=flags(r->output),after;
    if(s) r->sample=*s;
    r->output=kitchen_step(&r->control,&r->sample,now);
    if(r->manual && (uint32_t)(now-r->manual_since)>=r->manual_ms) {
        r->manual=false; runtime_log(r,now,3,0);
    }
    /* Fault and high temperature override manual ventilation-off. */
    if(r->manual) {
        r->output.lamp=r->manual_lamp;
        r->output.ventilation=r->manual_vent || r->output.hot || r->output.climate_fault;
    }
    after=flags(r->output); if(before!=after) runtime_log(r,now,1,after);
}
static bool same(const WireFrame *a,const WireFrame *b)
{
    return a->type==b->type && a->length==b->length &&
           a->length<=128 && memcmp(a->data,b->data,a->length)==0;
}
void runtime_command(KitchenRuntime *r,const WireFrame *q,WireFrame *a,uint32_t now)
{
    unsigned i; bool mutation=q->type==CMD_SET_CONFIG || q->type==CMD_MANUAL || q->type==CMD_ACK;
    memset(a,0,sizeof(*a)); a->type=(uint8_t)(q->type|0x80); a->seq=q->seq; a->length=1;
    r->commands++;
    if(q->length>128) { a->data[0]=RESULT_LENGTH; return; }
    for(i=0;i<4 && mutation;i++) {
        KitchenCached *c=&r->cache[i];
        if(c->used && (uint32_t)(now-c->at_ms)<10000 && c->request.seq==q->seq) {
            if(same(&c->request,q)) { *a=c->response; r->duplicates++; }
            else a->data[0]=RESULT_CONFLICT;
            return;
        }
    }
    switch(q->type) {
    case CMD_STATUS:
        if(q->length) { a->data[0]=RESULT_LENGTH; break; }
        runtime_tick(r,NULL,now); a->length=28;
        wire_put32(a->data+1,now); wire_put16(a->data+5,(uint16_t)r->sample.temp);
        wire_put16(a->data+7,r->sample.humid); wire_put16(a->data+9,r->sample.light);
        wire_put16(a->data+11,flags(r->output)); a->data[13]=(uint8_t)r->manual;
        a->data[14]=(uint8_t)r->dirty; wire_put32(a->data+15,now-r->sample.climate_ms);
        wire_put32(a->data+19,r->revision);
        wire_put32(a->data+23,now-r->sample.light_ms);
        a->data[27]=(uint8_t)((r->sample.climate_valid?1:0)|(r->sample.light_valid?2:0)); break;
    case CMD_GET_CONFIG:
        if(q->length) { a->data[0]=RESULT_LENGTH; break; }
        config_pack(&r->control.cfg,a->data+1); a->length=21; break;
    case CMD_SET_CONFIG: {
        KitchenConfig c; uint8_t current[CONFIG_BYTES];
        if(q->length!=20) { a->data[0]=RESULT_LENGTH; break; }
        if(!config_unpack(&c,q->data,q->length)) { a->data[0]=RESULT_VALUE; break; }
        config_pack(&r->control.cfg,current);
        if(memcmp(current,q->data,CONFIG_BYTES)==0) break;
        r->control.cfg=c; r->dirty=true; r->revision++;
        runtime_log(r,now,2,(uint16_t)r->revision); break;
    }
    case CMD_MANUAL: {
        uint32_t duration;
        if(q->length!=6) { a->data[0]=RESULT_LENGTH; break; }
        duration=wire_get32(q->data+2);
        if(q->data[0]>1 || q->data[1]>1 || duration>300000) { a->data[0]=RESULT_VALUE; break; }
        r->manual=duration!=0; r->manual_since=now; r->manual_ms=duration;
        r->manual_lamp=q->data[0]!=0; r->manual_vent=q->data[1]!=0;
        runtime_log(r,now,3,(uint16_t)r->manual); break;
    }
    case CMD_ACK:
        if(q->length) { a->data[0]=RESULT_LENGTH; break; }
        kitchen_ack(&r->control,now); runtime_log(r,now,4,0); break;
    case CMD_LOG: {
        unsigned index;
        if(q->length!=1) { a->data[0]=RESULT_LENGTH; break; }
        index=q->data[0];
        if(index>=r->log_count) { a->data[0]=RESULT_VALUE; break; }
        index=(r->log_head+16-1-index)%16; a->length=10;
        a->data[1]=(uint8_t)r->log_count; wire_put32(a->data+2,r->logs[index].at_ms);
        wire_put16(a->data+6,r->logs[index].event); wire_put16(a->data+8,r->logs[index].value); break;
    }
    default: a->data[0]=RESULT_UNSUPPORTED; break;
    }
    if(mutation) {
        KitchenCached *c=&r->cache[r->cache_next];
        c->used=true; c->at_ms=now; c->request=*q; c->response=*a; r->cache_next=(r->cache_next+1)%4;
    }
    runtime_tick(r,NULL,now);
}
