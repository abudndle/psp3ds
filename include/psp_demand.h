#ifndef PSP_DEMAND_H
#define PSP_DEMAND_H
#include "psp_perf.h"
#include <stdint.h>
/* An event callback may wake a guest thread. Return zero for success. */
typedef int (*PspDemandEventHandler)(void *opaque,uint32_t token,PspPerf *perf);
typedef struct {
 uint32_t instructions;
 uint32_t events;
 uint64_t skipped_idle_us;
} PspDemandStats;
/* Execute only runnable instructions, jump to due events while idle.
 * Return: 0 CPU budget consumed; 1 no more runnable work before deadline;
 * 2 bounded event budget reached; negative indicates failure.
 * Does not implement PSP thread switching, GPU or guest-cycle accounting.
 */
int psp_demand_run(PspPerf *perf,PspHle *hle,PspCpu *cpu,
                   uint32_t instruction_budget,uint32_t event_budget,
                   uint64_t deadline_us,PspDemandEventHandler handler,
                   void *opaque,PspDemandStats *stats);
#endif
