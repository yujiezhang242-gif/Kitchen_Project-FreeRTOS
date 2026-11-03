#include "kitchen.h"
#include <assert.h>
#include <stdio.h>
int main(void)
{
    Kitchen k;
    KitchenConfig c = kitchen_defaults();
    KitchenSample s = {0,0,250,500,1000,true,true};
    KitchenOutput o;
    assert(kitchen_init(&k,&c));
    o=kitchen_step(&k,&s,0); assert(!o.buzzer && !o.ventilation);
    s.temp=300; o=kitchen_step(&k,&s,1); assert(o.hot && o.buzzer && o.ventilation);
    kitchen_ack(&k,1); o=kitchen_step(&k,&s,2); assert(!o.buzzer && o.hot);
    s.temp=290; o=kitchen_step(&k,&s,3); assert(o.hot);
    s.climate_valid=false; o=kitchen_step(&k,&s,4); assert(o.sensor_fault && o.buzzer);
    kitchen_ack(&k,4); o=kitchen_step(&k,&s,60004); assert(o.buzzer);
    s.climate_valid=true; s.climate_ms=60005; s.light_ms=60005; s.temp=270;
    o=kitchen_step(&k,&s,60005); assert(!o.hot && !o.sensor_fault && !o.ventilation);
    s.humid=700; s.light=2800; o=kitchen_step(&k,&s,60006); assert(o.ventilation && o.lamp);
    s.humid=650; s.light=2600; o=kitchen_step(&k,&s,60007); assert(o.ventilation && o.lamp);
    s.humid=600; s.light=2400; o=kitchen_step(&k,&s,60008); assert(!o.ventilation && !o.lamp);
    o=kitchen_step(&k,&s,65005); assert(o.sensor_fault);
    assert(kitchen_init(&k,&c));
    s.climate_ms=UINT32_MAX-100; s.light_ms=UINT32_MAX-100; o=kitchen_step(&k,&s,100); assert(!o.sensor_fault);
    o=kitchen_step(&k,&s,5000); assert(o.sensor_fault);
    s.climate_ms=5000; s.light_ms=5000; s.humid=1001; o=kitchen_step(&k,&s,5000); assert(o.sensor_fault);
    c.temp_off=c.temp_on; assert(!kitchen_init(&k,&c));
    puts("PASS: thresholds, hysteresis, ACK, new alarm, timeout, recovery, wraparound, validation");
    return 0;
}
