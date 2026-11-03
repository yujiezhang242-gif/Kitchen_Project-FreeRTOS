#ifndef KITCHEN_TIMING_H
#define KITCHEN_TIMING_H
#include <stdbool.h>
#include <stdint.h>
typedef struct {
    uint32_t previous_start,budget,period_limit;
    uint32_t cycles,max_interval,max_execution,late_intervals,overruns;
    bool started;
} TimingStats;
void timing_init(TimingStats *s,uint32_t budget,uint32_t period_limit);
void timing_start(TimingStats *s,uint32_t now);
void timing_finish(TimingStats *s,uint32_t started_at,uint32_t finished_at);
#endif
