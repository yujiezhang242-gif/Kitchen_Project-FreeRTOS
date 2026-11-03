#ifndef KITCHEN_H
#define KITCHEN_H
#include <stdbool.h>
#include <stdint.h>
/* Monotonic uint32 milliseconds; all intervals < 2^31 ms.
 * Only the control task owns this context; other tasks exchange copies. */
typedef struct {
    int16_t temp_on, temp_off; /* 0.1 C */
    uint16_t humid_on, humid_off; /* 0.1 %RH */
    uint16_t dark_on, dark_off; /* high raw ADC means dark */
    uint32_t stale_ms, mute_ms;
} KitchenConfig;
typedef struct {
    uint32_t climate_ms, light_ms; /* last successful acquisition per channel */
    int16_t temp;
    uint16_t humid, light;
    bool climate_valid, light_valid;
} KitchenSample;
typedef struct {
    bool ventilation, lamp, buzzer, sensor_fault, hot;
    bool climate_fault, light_fault;
} KitchenOutput;
typedef struct {
    KitchenConfig cfg;
    KitchenOutput out;
    bool muted;
    uint32_t muted_since;
} Kitchen;
KitchenConfig kitchen_defaults(void);
bool kitchen_init(Kitchen *k,const KitchenConfig *cfg);
void kitchen_ack(Kitchen *k,uint32_t now);
KitchenOutput kitchen_step(Kitchen *k,const KitchenSample *s,uint32_t now);
#endif
