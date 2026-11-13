#include "psp_benchmark.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
int main(void){
 PspBenchResult r;
 assert(psp_benchmark_run(16,&r)==0);
 assert(r.executed_instructions==2048u);
 assert(r.requested_instructions==2048u);
 assert(r.checksum==2048u);
 assert(r.cpu_status==0);
 assert(psp_benchmark_run(64,&r)==0);
 assert(r.executed_instructions==8192u && r.checksum==8192u);
 assert(psp_benchmark_run(0,&r)==-1);
 assert(r.executed_instructions==0 && r.checksum==0);
 assert(psp_benchmark_run(65,&r)==-1);
 assert(psp_benchmark_run(1,0)==-1);
 puts("PASS: bounded, deterministic MIPS benchmark with correct guest instruction count");
 return 0;
}
