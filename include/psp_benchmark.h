#ifndef PSP_BENCHMARK_H
#define PSP_BENCHMARK_H
#include "psp_core.h"
#include <stdint.h>

/* Synthetic PSP MIPS interpreter benchmark used on host, DS, and DSi.
 * No real PSP game, real-time FPS claim, or emulator speed assumptions.
 * count is validated, deterministic instructions execute in guest RAM. */
#define PSP_BENCH_INSNS_PER_ROUND 128u
#define PSP_BENCH_MAX_ROUNDS 64u
typedef struct {
 uint32_t requested_instructions;
 uint32_t executed_instructions;
 uint32_t checksum;
 uint32_t failed_round;
 int cpu_status;
} PspBenchResult;
int psp_benchmark_run(uint32_t rounds, PspBenchResult *out);
#endif
