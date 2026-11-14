#include <3ds.h>
#include <stdint.h>
#include <stdio.h>
#include "psp_runtime_guest.h"
#include "psp_benchmark.h"

/* The real shared MIPS/HLE scheduler, not a precomputed instruction count.
 * Runs only an original synthetic syscall + ADDIU fixture at application
 * startup. PSP games still require upstream guest memory/HLE integration. */
#define DEMO_BASE UINT32_C(0x08800000)
#define DEMO_MAX_INSNS (PSP_BENCH_MAX_ROUNDS*PSP_BENCH_INSNS_PER_ROUND)
static uint8_t guest_code[(DEMO_MAX_INSNS+1u)*4u];
static PspPerf perf;
static PspHle hle;
static PspCpu cpu;

typedef struct {
 TickCounter ticks;
 uint64_t elapsed_us;
} NativeWallClock;

static uint64_t wall_clock_us(void *context){
 NativeWallClock *clock=(NativeWallClock*)context;
 osTickCounterUpdate(&clock->ticks);
 double elapsed_ms=osTickCounterRead(&clock->ticks);
 if(elapsed_ms>0.0 && elapsed_ms<1000000.0)
  clock->elapsed_us+=(uint64_t)(elapsed_ms*1000.0+0.5);
 return clock->elapsed_us;
}
static void put_le32(uint8_t *where,uint32_t word){
 for(unsigned i=0;i<4;i++)where[i]=(uint8_t)(word>>(i*8u));
}
int psp_3ds_guest_runtime_demo(PspTarget target){
 if(target!=PSP_TARGET_OLD3DS && target!=PSP_TARGET_NEW3DS)return -1;
 PspRuntimeTuning tuning;
 if(psp_runtime_tuning_choose(target,&tuning)!=0 ||
    tuning.cpu_slice_current>DEMO_MAX_INSNS)return -1;
 const uint32_t budget=tuning.cpu_slice_current;
 /* First guest opcode: syscall 9 (bound only for synthetic HLE testing).
  * All following opcodes: ADDIU t0,t0,1. This validates two consecutive
  * scheduler calls with no invented instruction counts or game rendering. */
 put_le32(guest_code,(9u<<6u)|12u);
 for(uint32_t i=0;i<budget;i++)
  put_le32(guest_code+(i+1u)*4u,UINT32_C(0x25080001));
 psp_perf_init(&perf);
 psp_hle_init(&hle);
 psp_cpu_init(&cpu,guest_code,(size_t)(budget+1u)*4u,DEMO_BASE,DEMO_BASE);
 if(psp_hle_bind(&hle,9u,PSP_HLE_SYSTEM_TIME_LOW)!=0)return -1;
 hle.uptime_us=12345u;

 NativeWallClock timer={0};
 osTickCounterStart(&timer.ticks);
 PspRuntimeGuestStep first={0},second={0};
 int status=psp_runtime_guest_step(&tuning,&perf,&hle,&cpu,0,
                                   wall_clock_us,&timer,&first);
 if(status!=0||first.executed!=1u||cpu.gpr[2]!=12345u ||
    cpu.pc!=DEMO_BASE+4u)return -1;
 status=psp_runtime_guest_step(&tuning,&perf,&hle,&cpu,0,
                               wall_clock_us,&timer,&second);
 if(status!=0||second.executed!=budget||cpu.gpr[8]!=budget||
    cpu.pc!=DEMO_BASE+(budget+1u)*4u ||
    perf.now_us!=0 || hle.uptime_us!=12345u)return -1;
 printf("HLE guest: %lu MIPS insns OK\n",(unsigned long)second.executed);
 printf("Guest slice wall: %lu us\n",(unsigned long)second.wall_elapsed_us);
 printf("Next guest slice: %lu\n",(unsigned long)second.next_budget);
 return 0;
}
