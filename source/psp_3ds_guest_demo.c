#include <3ds.h>
#include <stdint.h>
#include <stdio.h>
#include "psp_on_demand_guest.h"
#include "psp_benchmark.h"

/* Real shared MIPS/HLE execution with demand-only synthetic backend work.
 * No proprietary game code, PSP GE display list or retail audio output.
 * Unneeded GPU/audio/UMD backends are NOT initialized. */
#define DEMO_BASE UINT32_C(0x08800000)
#define DEMO_MAX_INSNS (PSP_BENCH_MAX_ROUNDS*PSP_BENCH_INSNS_PER_ROUND)
static uint8_t guest_code[(DEMO_MAX_INSNS+1u)*4u];
static PspPerf perf;
static PspHle hle;
static PspCpu cpu;
typedef struct {
 TickCounter ticks;
} NativeWallClock;
typedef struct {
 uint32_t began[PSP_LAZY_SUBSYSTEMS];
 uint32_t worked[PSP_LAZY_SUBSYSTEMS];
 uint32_t ended[PSP_LAZY_SUBSYSTEMS];
} NativeLazyCounters;
static uint64_t wall_clock_us(void *context){
 NativeWallClock *clock=(NativeWallClock*)context;
 osTickCounterUpdate(&clock->ticks);
 double elapsed_ms=osTickCounterRead(&clock->ticks);
 if(elapsed_ms>0.0 && elapsed_ms<1000000.0)
  return (uint64_t)(elapsed_ms*1000.0+0.5);
 return 0;
}
static int begin(void *ctx,PspLazySubsystem sub){
 ((NativeLazyCounters*)ctx)->began[sub]++;
 return 0;
}
static int work(void *ctx,PspLazySubsystem sub){
 ((NativeLazyCounters*)ctx)->worked[sub]++;
 return 0;
}
static void end(void *ctx,PspLazySubsystem sub){
 ((NativeLazyCounters*)ctx)->ended[sub]++;
}
static void put_le32(uint8_t *where,uint32_t word){
 for(unsigned i=0;i<4;i++)where[i]=(uint8_t)(word>>(i*8u));
}
int psp_3ds_guest_runtime_demo(PspTarget target){
 if(target!=PSP_TARGET_OLD3DS&&target!=PSP_TARGET_NEW3DS)return -1;
 PspRuntimeTuning tuning;
 PspPlayCutbacks cuts;
 PspLazyBackends lazy={0};
 if(psp_runtime_tuning_choose(target,&tuning)!=0 ||
    psp_play_cutbacks_init(&cuts,target)!=0 ||
    psp_lazy_backends_init(&lazy,target)!=0 ||
    tuning.cpu_slice_current>DEMO_MAX_INSNS)return -1;
 const uint32_t budget=tuning.cpu_slice_current;
 put_le32(guest_code,(9u<<6u)|12u);
 for(uint32_t i=0;i<budget;i++)
  put_le32(guest_code+(i+1u)*4u,UINT32_C(0x25080001));
 psp_perf_init(&perf);
 psp_hle_init(&hle);
 psp_cpu_init(&cpu,guest_code,(size_t)(budget+1u)*4u,DEMO_BASE,DEMO_BASE);
 if(psp_hle_bind(&hle,9u,PSP_HLE_SYSTEM_TIME_LOW)!=0)return -1;
 hle.uptime_us=12345u;
 NativeLazyCounters counters={0};
 PspLazyBackendOps ops={begin,work,end,&counters};
 NativeWallClock timer={0};
 osTickCounterStart(&timer.ticks);
 PspOnDemandGuestStep first={0},second={0};
 int ok=1;
 int status=psp_on_demand_guest_step(&tuning,&perf,&hle,&cpu,
                   &lazy,&ops,&cuts,0,wall_clock_us,&timer,&first);
 if(status!=0 || first.guest.executed!=1u ||
    first.backend_called || lazy.initialized_count ||
    cpu.gpr[2]!=12345u || cpu.pc!=DEMO_BASE+4u)ok=0;
 if(ok && (psp_lazy_backends_request(&lazy,PSP_LAZY_GPU,1u)!=0 ||
           psp_lazy_backends_request(&lazy,PSP_LAZY_AUDIO,1u)!=0 ||
           psp_lazy_backends_request(&lazy,PSP_LAZY_UMD,1u)!=0))ok=0;
 if(ok){
  status=psp_on_demand_guest_step(&tuning,&perf,&hle,&cpu,
                   &lazy,&ops,&cuts,0,wall_clock_us,&timer,&second);
  if(status!=0||second.guest.executed!=budget||cpu.gpr[8]!=budget||
     cpu.pc!=DEMO_BASE+(budget+1u)*4u||perf.now_us!=0 ||
     hle.uptime_us!=12345u||second.backend_completed!=3u ||
     second.backend_remaining!=0u)ok=0;
 }
 PspLazyShutdown report;
 if(psp_lazy_backends_close(&lazy,&ops,&report)!=0 ||
    report.pending_units!=0u || report.initialized_services!=3u)ok=0;
 for(unsigned i=0;i<PSP_LAZY_SUBSYSTEMS;i++)
  if(counters.began[i]!=1u||counters.worked[i]!=1u||
     counters.ended[i]!=1u)ok=0;
 printf("MIPS/HLE on-demand: %s\n",ok?"PASS":"FAILED");
 if(ok){
  printf("Guest instructions: %lu\n",
         (unsigned long)second.guest.executed);
  printf("Guest slice: %lu us; next %lu insns\n",
         (unsigned long)second.guest.wall_elapsed_us,
         (unsigned long)second.guest.next_budget);
  printf("GPU/audio/UMD init only on request\n");
 }
 return ok?0:-1;
}
