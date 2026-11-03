#include "health.h"
void health_init(KitchenHealth *h,uint32_t now)
{
    unsigned i; h->started_ms=now; h->grace_ms=3000;
    for(i=0;i<5;i++) h->deadline_ms[i]=1500;
}
uint32_t health_stale(const KitchenHealth *h,const uint32_t last[5],uint32_t now)
{
    unsigned i; uint32_t mask=0;
    if((uint32_t)(now-h->started_ms)<h->grace_ms) return 0;
    for(i=0;i<5;i++) if((uint32_t)(now-last[i])>h->deadline_ms[i]) mask|=UINT32_C(1)<<i;
    return mask;
}
