#include "psp_perf.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>
static int reads;
static int read_sector(void *ctx,uint32_t lba,uint8_t *out){(void)ctx;reads++;memset(out,(int)(lba&255u),PSP_PERF_SECTOR_SIZE);return lba==999?-1:0;}
int main(void){
 PspPerf p;psp_perf_init(&p);
 assert(psp_perf_request_subsystem(&p,PSP_SUBSYSTEM_GPU)==1);
 assert(psp_perf_request_subsystem(&p,PSP_SUBSYSTEM_GPU)==0);
 assert(psp_perf_request_subsystem(&p,(PspSubsystem)99)==-1);
 assert(psp_perf_schedule(&p,30,1)==0);
 assert(psp_perf_schedule(&p,10,2)==0);
 assert(psp_perf_schedule(&p,10,3)==0);
 assert(psp_perf_schedule(&p,25,2)==-1);
 psp_perf_wait_until(&p,40);
 assert(psp_perf_idle_advance(&p,50)==2 && p.now_us==10);
 assert(psp_perf_idle_advance(&p,50)==3 && p.now_us==10);
 assert(psp_perf_idle_advance(&p,50)==1 && p.now_us==30);
 assert(psp_perf_idle_advance(&p,50)==0 && p.now_us==40 && p.thread_state==PSP_THREAD_RUNNABLE);
 assert(psp_perf_idle_advance(&p,100)==0 && p.now_us==40);
 assert(psp_perf_record_block(&p,0x100,3)==0);
 assert(psp_perf_record_block(&p,0x100,3)==0);
 assert(psp_perf_record_block(&p,0x100,3)==1);
 assert(psp_perf_record_block(&p,0x100,3)==1);
 assert(psp_perf_record_block(&p,0x200,3)==0);
 assert(psp_perf_texture_seen(&p,17,1)==0);
 assert(psp_perf_texture_seen(&p,17,1)==1);
 assert(psp_perf_texture_seen(&p,17,2)==0);
 psp_perf_invalidate_textures(&p);
 assert(psp_perf_texture_seen(&p,17,2)==0);
 uint8_t sector[PSP_PERF_SECTOR_SIZE];
 assert(psp_perf_read_sector(&p,7,read_sector,NULL,sector)==0 && reads==1 && sector[1]==7);
 assert(psp_perf_read_sector(&p,7,read_sector,NULL,sector)==1 && reads==1);
 /* Same sector number from another reader context is not the same disc. */
 int another_disc=5;
 assert(psp_perf_read_sector(&p,7,read_sector,&another_disc,sector)==0 && reads==2);
 assert(psp_perf_read_sector(&p,7,read_sector,&another_disc,sector)==1 && reads==2);
 assert(psp_perf_read_sector(&p,7,read_sector,NULL,sector)==0 && reads==3);
 for(uint32_t j=8;j<13;j++)assert(psp_perf_read_sector(&p,j,read_sector,NULL,sector)==0);
 assert(psp_perf_read_sector(&p,7,read_sector,NULL,sector)==0);
 reads=0;
 assert(psp_perf_read_sector(&p,999,read_sector,NULL,sector)==-2);
 assert(psp_perf_read_sector(&p,999,read_sector,NULL,sector)==-2 && reads==2);
 psp_perf_profile(&p,"UNKNOWN");assert(p.frameskip==0 && p.allow_texture_cache==1);
 uint8_t ram[64]={0};PspCpu cpu;PspHle hle;psp_cpu_init(&cpu,ram,sizeof ram,0x08800000,0x08800000);
 psp_hle_init(&hle);
 psp_perf_wait_until(&p,90);assert(psp_perf_step(&p,&hle,&cpu)==2 && cpu.pc==0x08800000);
 assert(psp_perf_idle_advance(&p,100)==0 && p.now_us==90);
 assert(psp_perf_step(&p,&hle,&cpu)==0 && cpu.pc==0x08800004);
 puts("PASS: idle events, lazy initialization, hot blocks, caches and safe profiles");return 0;
}
