#include "psp_guarded_prx.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#define BASE UINT32_C(0x08800000)
#define MB (1024u*1024u)
#define KB 1024u
static void w16(uint8_t *p,uint16_t v){p[0]=(uint8_t)v;p[1]=(uint8_t)(v>>8);}
static void w32(uint8_t *p,uint32_t v){
 for(unsigned i=0;i<4;i++)p[i]=(uint8_t)(v>>(i*8u));
}
static uint32_t r32(const uint8_t *p){
 return (uint32_t)p[0]|(uint32_t)p[1]<<8 |
        (uint32_t)p[2]<<16|(uint32_t)p[3]<<24;
}
static void fixture(uint8_t elf[512]){
 memset(elf,0,512);
 memcpy(elf,"\177ELF",4);elf[4]=1;elf[5]=1;elf[6]=1;
 w16(elf+16,0xffa0u);w16(elf+18,8u);
 w32(elf+20,1u);w32(elf+24,0u);w32(elf+28,52u);
 w16(elf+40,52u);w16(elf+42,32u);w16(elf+44,3u);
 w32(elf+52,1u);w32(elf+56,256u);w32(elf+60,0u);
 w32(elf+68,32u);w32(elf+72,48u);w32(elf+76,5u);
 w32(elf+84,1u);w32(elf+88,320u);w32(elf+92,64u);
 w32(elf+100,16u);w32(elf+104,32u);w32(elf+108,6u);
 w32(elf+116,0x700000a0u);w32(elf+120,384u);
 w32(elf+132,16u);w32(elf+136,16u);
 w32(elf+256,0x25080001u);
 w32(elf+260,64u);
 w32(elf+320,16u);
 w32(elf+384,4u);w32(elf+388,2u);
 w32(elf+392,0u);w32(elf+396,(1u<<8)|2u);
}
static void choose(PspTarget model,PspMemoryGuardPolicy *policy){
 if(model<=PSP_TARGET_DSI){
  PspDsMemoryObservation obs={0};
  obs.largest_touched_heap_allocation=model==PSP_TARGET_DSI?2u*MB:512u*KB;
  if(model==PSP_TARGET_DS_SLOT2){
   obs.slot2_reported_bytes=8u*MB;
   obs.slot2_sampled_verified=obs.slot2_single_bank=1;
  }
  PspDsMemoryPlan p;
  assert(psp_ds_memory_plan(model,&obs,&p)==0);
  assert(psp_memory_guard_policy_ds(&p,policy)==0);
 }else{
  Psp3dsMemoryPlan p;
  assert(psp_3ds_memory_plan(model,
         model==PSP_TARGET_NEW3DS?124u*MB:64u*MB,
         model==PSP_TARGET_NEW3DS?110u*MB:60u*MB,&p)==0);
  assert(psp_memory_guard_policy_3ds(&p,policy)==0);
 }
}
static void all_targets_small_executable(void){
 uint8_t elf[512];
 fixture(elf);
 for(int model=PSP_TARGET_DS;model<PSP_TARGET_COUNT;model++){
  PspMemoryGuardPolicy policy;
  choose((PspTarget)model,&policy);
  PspMemoryGuard guard={0};
  assert(psp_memory_guard_init(&guard,&policy,NULL,NULL,NULL)==0);
  for(unsigned cycle=0;cycle<16u;cycle++){
   PspGuardedPrx image={0};
   assert(psp_guarded_prx_load(&guard,elf,sizeof elf,BASE,&image)==0);
   assert(image.allocated_bytes==4096u);
   assert(image.image.entry==BASE && image.image.image_bytes==96u);
   assert(image.cpu.pc==BASE);
   assert(r32(image.cpu.ram+4)==BASE+64u);
   assert(psp_cpu_step(&image.cpu)==0&&image.cpu.gpr[8]==1u);
   assert(guard.outstanding_allocations==1u);
   assert(psp_guarded_prx_unload(&guard,&image)==0);
   assert(image.guest_ram==NULL);
   assert(guard.outstanding_allocations==0u);
   assert(psp_guarded_prx_unload(&guard,&image)==-1);
  }
  PspMemoryGuardReport report;
  assert(psp_memory_guard_close(&guard,&report)==0);
  assert(report.successful_allocations==16u &&
         report.successful_releases==16u &&
         report.reclaimed_allocations==0u);
 }
}
static void stress_large_image_and_fail_closed(void){
 /* An original synthetic image with Jetpack-like mapped size, NOT any
  * proprietary executable data. This exceeds both DS/DSi quotas. */
 uint8_t elf[512];
 fixture(elf);
 /* Make one wide segment, and retain the two relocation targets inside it. */
 w32(elf+72,6320916u);
 w32(elf+84,0u);  /* remove overlapping second PT_LOAD */
 w32(elf+396,2u); /* map data relocation to single segment 0 */
 for(int model=PSP_TARGET_DS;model<PSP_TARGET_COUNT;model++){
  PspMemoryGuardPolicy policy;
  choose((PspTarget)model,&policy);
  PspMemoryGuard guard={0};
  assert(psp_memory_guard_init(&guard,&policy,NULL,NULL,NULL)==0);
  PspGuardedPrx image={0};
  const int status=psp_guarded_prx_load(&guard,elf,sizeof elf,BASE,&image);
  if(model<=PSP_TARGET_DSI){
   assert(status==-3);
   assert(guard.denied_requests==1u);
  }else{
   assert(status==0);
   assert(image.allocated_bytes>6320916u-4096u);
   assert(image.image.image_bytes==6320916u);
   assert(image.cpu.pc==BASE);
   assert(psp_guarded_prx_unload(&guard,&image)==0);
  }
  assert(image.guest_ram==NULL && guard.allocated_bytes==0u);
  PspMemoryGuardReport report;
  assert(psp_memory_guard_close(&guard,&report)==0);
  assert(report.reclaimed_allocations==0u);
 }
}
static void errors_and_auto_reclaim(void){
 uint8_t elf[512];fixture(elf);
 PspMemoryGuardPolicy policy;
 choose(PSP_TARGET_OLD3DS,&policy);
 PspMemoryGuard guard={0};
 assert(psp_memory_guard_init(&guard,&policy,NULL,NULL,NULL)==0);
 PspGuardedPrx image={0};
 w32(elf+384,UINT32_C(0xdeadbeef));
 assert(psp_guarded_prx_load(&guard,elf,sizeof elf,BASE,&image)==-4);
 assert(image.guest_ram==NULL && guard.allocated_bytes==0u);
 fixture(elf);
 w32(elf+68,UINT32_C(0x02000001)); /* load bigger than max image */
 assert(psp_guarded_prx_load(&guard,elf,sizeof elf,BASE,&image)==-2);
 assert(guard.allocated_bytes==0u);
 fixture(elf);
 assert(psp_guarded_prx_load(&guard,elf,sizeof elf,BASE,&image)==0);
 assert(psp_guarded_prx_load(&guard,elf,sizeof elf,BASE,&image)==-1);
 assert(psp_guarded_prx_unload(&guard,&image)==0);
 assert(psp_guarded_prx_unload(&guard,&image)==-1);
 assert(psp_guarded_prx_load(&guard,elf,sizeof elf,BASE,&image)==0);
 /* Session error before unload: guard must reclaim the whole PRX memory. */
 PspMemoryGuardReport report;
 assert(psp_memory_guard_close(&guard,&report)==0);
 assert(report.reclaimed_allocations==1u);
 assert(report.reclaimed_bytes==4096u);
 assert(report.end_outstanding_bytes==4096u);
}
int main(void){
 all_targets_small_executable();
 stress_large_image_and_fail_closed();
 errors_and_auto_reclaim();
 puts("PASS: guarded PSP PRX loads, rejects 6.32MB image on DS/DSi, cleans game RAM across all five models");
 return 0;
}
