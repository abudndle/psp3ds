#include <nds.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include "psp_ds_memory.h"

/* A successful scratch allocation is not a free-memory inventory.
 * Allocate/touch/release at most the existing hot-memory budget while
 * preserving the Nintendo OS, graphics, audio and both CPU stacks.
 * On DSi in DS-compatibility mode, the caller supplies PSP_TARGET_DS. */
static uint32_t probe_temporary_heap(PspTarget target){
 PspPlatformBudget policy;
 if(psp_platform_budget_choose(target,&policy)!=0)return 0;
 uint32_t candidate=policy.hot_ram_budget;
 while(candidate>=64u*PSP_DS_KIB){
  uint8_t *buffer=(uint8_t*)malloc(candidate);
  if(buffer){
   volatile uint8_t *touch=(volatile uint8_t*)buffer;
   for(uint32_t pos=0;pos<candidate;pos+=PSP_DS_MEMORY_GRANULE)
    touch[pos]=(uint8_t)(pos/PSP_DS_MEMORY_GRANULE);
   touch[candidate-1u]=0x5au;
   free(buffer);
   return candidate;
  }
  candidate/=2u;
 }
 return 0;
}
void psp_nds_memory_report(PspTarget target,
                           uint32_t sampled_slot2_bytes,
                           int sampled_slot2_verified){
 PspDsMemoryObservation observation={0};
 PspDsMemoryPlan plan;
 observation.largest_touched_heap_allocation=probe_temporary_heap(target);
 if(target==PSP_TARGET_DS_SLOT2 && sampled_slot2_verified){
  observation.slot2_reported_bytes=sampled_slot2_bytes;
  observation.slot2_sampled_verified=1;
  observation.slot2_single_bank=1;
  /* The earlier test_slot2_ram() closes the device before returning.
   * Never expose nonzero ACTIVE cold RAM until it is reopened. */
  observation.slot2_currently_open=0;
 }
 if(psp_ds_memory_plan(target,&observation,&plan)!=0){
  printf("DS RAM policy: unavailable\n");
  return;
 }
 printf("Main RAM %lu MiB; hot cap %lu KiB\n",
        (unsigned long)(plan.installed_main_bytes/PSP_DS_MIB),
        (unsigned long)(plan.platform_hot_budget_bytes/PSP_DS_KIB));
 printf("Touched guest heap: %lu KiB\n",
        (unsigned long)(plan.usable_hot_bytes/PSP_DS_KIB));
 if(plan.slot2_sampled_verified){
  printf("Pak %lu MiB reported; cold %lu KiB\n",
         (unsigned long)(plan.reported_slot2_bytes/PSP_DS_MIB),
         (unsigned long)(plan.potential_cold_bytes/PSP_DS_KIB));
  printf("Pak active: %s (closed)\n",
         plan.active_cold_bytes?"YES":"NO");
 }else{
  printf("Pak cold RAM: not available\n");
 }
 printf("PSP guest RAM not allocated\n");
}
