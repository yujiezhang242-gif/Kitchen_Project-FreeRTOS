#ifndef KITCHEN_HEALTH_H
#define KITCHEN_HEALTH_H
#include <stdbool.h>
#include <stdint.h>
#define HEALTH_WORKERS 5u
typedef struct { uint32_t started_ms, grace_ms, deadline_ms[5]; } KitchenHealth;
void health_init(KitchenHealth *h,uint32_t now);
/* Returns stale-worker bit mask; supervisor itself is protected by watchdog. */
uint32_t health_stale(const KitchenHealth *h,const uint32_t last_ms[5],uint32_t now);
#endif
