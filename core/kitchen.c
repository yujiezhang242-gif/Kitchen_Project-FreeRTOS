#include "kitchen.h"
#include <string.h>
KitchenConfig kitchen_defaults(void)
{
    KitchenConfig c={300,270,700,600,2800,2400,5000,60000}; return c;
}
bool kitchen_init(Kitchen *k,const KitchenConfig *c)
{
    if(!k || !c || c->temp_on<=c->temp_off || c->temp_off < -400 || c->temp_on>1250 ||
       c->humid_on<=c->humid_off || c->humid_on>1000 || c->dark_on<=c->dark_off || c->dark_on>4095 ||
       !c->stale_ms || c->stale_ms>=UINT32_C(0x80000000) || !c->mute_ms || c->mute_ms>=UINT32_C(0x80000000)) return false;
    memset(k,0,sizeof(*k)); k->cfg=*c; return true;
}
void kitchen_ack(Kitchen *k,uint32_t now)
{
    if(k->out.hot || k->out.sensor_fault) { k->muted=true; k->muted_since=now; }
}
KitchenOutput kitchen_step(Kitchen *k,const KitchenSample *s,uint32_t now)
{
    bool old_hot=k->out.hot,old_climate=k->out.climate_fault,old_light=k->out.light_fault;
    bool cf=!s || !s->climate_valid,lf=!s || !s->light_valid;
    if(!cf) cf=(uint32_t)(now-s->climate_ms)>=k->cfg.stale_ms || s->temp < -400 || s->temp>1250 || s->humid>1000;
    if(!lf) lf=(uint32_t)(now-s->light_ms)>=k->cfg.stale_ms || s->light>4095;
    k->out.climate_fault=cf; k->out.light_fault=lf; k->out.sensor_fault=cf||lf;
    if(cf) k->out.ventilation=true;
    else {
        if(s->temp>=k->cfg.temp_on) k->out.hot=true;
        else if(s->temp<=k->cfg.temp_off) k->out.hot=false;
        if(k->out.hot || s->humid>=k->cfg.humid_on) k->out.ventilation=true;
        else if(s->humid<=k->cfg.humid_off) k->out.ventilation=false;
    }
    /* A failed light channel retains only the lamp state; climate still works. */
    if(!lf) {
        if(s->light>=k->cfg.dark_on) k->out.lamp=true;
        else if(s->light<=k->cfg.dark_off) k->out.lamp=false;
    }
    if((!old_hot && k->out.hot) || (!old_climate && cf) || (!old_light && lf) ||
       (k->muted && (uint32_t)(now-k->muted_since)>=k->cfg.mute_ms)) k->muted=false;
    if(!k->out.hot && !k->out.sensor_fault) k->muted=false;
    k->out.buzzer=(k->out.hot || k->out.sensor_fault) && !k->muted;
    return k->out;
}
