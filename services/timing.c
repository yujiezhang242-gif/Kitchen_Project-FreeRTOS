#include "timing.h"
#include <string.h>
void timing_init(TimingStats *s,uint32_t budget,uint32_t limit)
{ memset(s,0,sizeof(*s)); s->budget=budget; s->period_limit=limit; }
void timing_start(TimingStats *s,uint32_t now)
{
    if(s->started) {
        uint32_t interval=now-s->previous_start;
        if(interval>s->max_interval) s->max_interval=interval;
        if(interval>s->period_limit) s->late_intervals++;
    }
    s->previous_start=now; s->started=true; s->cycles++;
}
void timing_finish(TimingStats *s,uint32_t start,uint32_t end)
{
    uint32_t execution=end-start;
    if(execution>s->max_execution) s->max_execution=execution;
    if(execution>s->budget) s->overruns++;
}
