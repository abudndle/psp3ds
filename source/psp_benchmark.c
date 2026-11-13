#include "psp_benchmark.h"
#include <string.h>
/* Repeated ADDIU t0, t0, 1; deliberately measure the real shared
 * interpreter, not a precomputed answer or host/ARM native arithmetic. */
int psp_benchmark_run(uint32_t rounds,PspBenchResult *out) {
 if(out)memset(out,0,sizeof(*out));
 if(!out || !rounds || rounds>PSP_BENCH_MAX_ROUNDS)return -1;
 uint8_t guest[PSP_BENCH_INSNS_PER_ROUND*4u];
 for(unsigned i=0;i<PSP_BENCH_INSNS_PER_ROUND;i++) {
  guest[i*4]=1;guest[i*4+1]=0;guest[i*4+2]=8;guest[i*4+3]=0x25;
 }
 const uint32_t base=UINT32_C(0x08800000);
 out->requested_instructions=rounds*PSP_BENCH_INSNS_PER_ROUND;
 for(uint32_t round=0;round<rounds;round++){
  PspCpu cpu;
  psp_cpu_init(&cpu,guest,sizeof guest,base,base);
  for(unsigned i=0;i<PSP_BENCH_INSNS_PER_ROUND;i++) {
   int status=psp_cpu_step(&cpu);
   if(status) {
    out->failed_round=round;
    out->cpu_status=status;
    return -2;
   }
   out->executed_instructions++;
  }
  if(cpu.gpr[8]!=PSP_BENCH_INSNS_PER_ROUND ||
     cpu.pc!=base+sizeof guest)return -3;
  out->checksum+=cpu.gpr[8];
 }
 return out->executed_instructions==out->requested_instructions &&
        out->checksum==out->requested_instructions?0:-3;
}
