#include "psp_ds_memory.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

#define MB PSP_DS_MIB
#define KB PSP_DS_KIB
static void normal_ds(void){
 PspDsMemoryObservation observation={0};
 PspDsMemoryPlan plan;
 assert(psp_ds_memory_plan(PSP_TARGET_DS,&observation,&plan)==0);
 assert(plan.installed_main_bytes==4u*MB);
 assert(plan.combined_reported_bytes==4u*MB);
 assert(plan.platform_hot_budget_bytes==512u*KB);
 assert(plan.confirmed_temporary_heap_bytes==0u);
 assert(plan.usable_hot_bytes==0u);
 assert(!plan.active_cold_bytes && !plan.potential_cold_bytes);
 assert(plan.texture_budget_bytes==64u*KB);
 assert(plan.io_budget_bytes==64u*KB);
 observation.largest_touched_heap_allocation=128u*KB;
 assert(psp_ds_memory_plan(PSP_TARGET_DS,&observation,&plan)==0);
 assert(plan.usable_hot_bytes==128u*KB);
 observation.largest_touched_heap_allocation=1025u*KB;
 assert(psp_ds_memory_plan(PSP_TARGET_DS,&observation,&plan)==0);
 assert(plan.usable_hot_bytes==512u*KB);
 /* A physical 4 MiB address space does not imply 4 MiB heap. */
 observation.largest_touched_heap_allocation=4u*MB+1u;
 assert(psp_ds_memory_plan(PSP_TARGET_DS,&observation,&plan)==-1);
 assert(plan.installed_main_bytes==0u);
}
static void official_pak_and_reopen(void){
 PspDsMemoryObservation observation={512u*KB,8u*MB,1,1,1};
 PspDsMemoryPlan plan;
 assert(psp_ds_memory_plan(PSP_TARGET_DS_SLOT2,&observation,&plan)==0);
 assert(plan.installed_main_bytes==4u*MB);
 assert(plan.reported_slot2_bytes==8u*MB);
 assert(plan.combined_reported_bytes==12u*MB);
 assert(plan.platform_hot_budget_bytes==512u*KB);
 assert(plan.usable_hot_bytes==512u*KB);
 assert(plan.potential_cold_bytes==2u*MB);
 assert(plan.active_cold_bytes==2u*MB);
 assert(plan.slot2_sampled_verified && plan.slot2_open_for_use);
 /* The native demo closes Slot-2 after probing: no active cold RAM. */
 observation.slot2_currently_open=0;
 assert(psp_ds_memory_plan(PSP_TARGET_DS_SLOT2,&observation,&plan)==0);
 assert(plan.potential_cold_bytes==2u*MB);
 assert(plan.active_cold_bytes==0u);
 assert(plan.slot2_sampled_verified && !plan.slot2_open_for_use);
 /* Third-party 32 MiB Slot-2 may be reported but is NOT a normal heap,
  * and PSP3DS keeps its separate conservative 2 MiB cold-data cap. */
 observation.slot2_reported_bytes=32u*MB;
 assert(psp_ds_memory_plan(PSP_TARGET_DS_SLOT2,&observation,&plan)==0);
 assert(plan.combined_reported_bytes==36u*MB);
 assert(plan.potential_cold_bytes==2u*MB);
 assert(plan.usable_hot_bytes==512u*KB);
 observation.slot2_reported_bytes=4u*KB;
 assert(psp_ds_memory_plan(PSP_TARGET_DS_SLOT2,&observation,&plan)==0);
 assert(plan.potential_cold_bytes==4u*KB);
}
static void dsi(void){
 PspDsMemoryObservation observation={3u*MB,0,0,0,0};
 PspDsMemoryPlan plan;
 assert(psp_ds_memory_plan(PSP_TARGET_DSI,&observation,&plan)==0);
 assert(plan.installed_main_bytes==16u*MB);
 assert(plan.combined_reported_bytes==16u*MB);
 assert(plan.platform_hot_budget_bytes==2u*MB);
 assert(plan.usable_hot_bytes==2u*MB);
 assert(!plan.active_cold_bytes && !plan.potential_cold_bytes);
 assert(plan.texture_budget_bytes==128u*KB);
 assert(plan.io_budget_bytes==128u*KB);
 /* DSi in DS-compatibility mode chooses PSP_TARGET_DS, and thus
  * cannot access the extra 12 MiB of physical RAM. */
 observation.largest_touched_heap_allocation=512u*KB;
 assert(psp_ds_memory_plan(PSP_TARGET_DS,&observation,&plan)==0);
 assert(plan.installed_main_bytes==4u*MB);
 assert(plan.usable_hot_bytes==512u*KB);
 observation.largest_touched_heap_allocation=16u*MB+1u;
 assert(psp_ds_memory_plan(PSP_TARGET_DSI,&observation,&plan)==-1);
 assert(plan.installed_main_bytes==0u);
}
static void unsupported_or_unverified(void){
 PspDsMemoryObservation observation={512u*KB,8u*MB,0,1,1};
 PspDsMemoryPlan plan;
 memset(&plan,0xa5,sizeof plan);
 assert(psp_ds_memory_plan(PSP_TARGET_DS_SLOT2,&observation,&plan)==-2);
 assert(plan.installed_main_bytes==0u);
 observation.slot2_sampled_verified=1;
 observation.slot2_single_bank=0;
 assert(psp_ds_memory_plan(PSP_TARGET_DS_SLOT2,&observation,&plan)==-2);
 observation.slot2_single_bank=1;
 observation.slot2_reported_bytes=4095u;
 assert(psp_ds_memory_plan(PSP_TARGET_DS_SLOT2,&observation,&plan)==-2);
 observation.slot2_reported_bytes=8193u;
 assert(psp_ds_memory_plan(PSP_TARGET_DS_SLOT2,&observation,&plan)==-2);
 observation.slot2_reported_bytes=33u*MB;
 assert(psp_ds_memory_plan(PSP_TARGET_DS_SLOT2,&observation,&plan)==-2);
 observation.slot2_reported_bytes=8u*MB;
 assert(psp_ds_memory_plan(PSP_TARGET_DS,&observation,&plan)==-1);
 assert(psp_ds_memory_plan(PSP_TARGET_DSI,&observation,&plan)==-1);
 assert(psp_ds_memory_plan(PSP_TARGET_OLD3DS,&observation,&plan)==-1);
 assert(psp_ds_memory_plan(PSP_TARGET_NEW3DS,&observation,&plan)==-1);
 assert(psp_ds_memory_plan(PSP_TARGET_COUNT,&observation,&plan)==-1);
 assert(psp_ds_memory_plan(PSP_TARGET_DS,&observation,NULL)==-1);
 assert(psp_ds_memory_plan(PSP_TARGET_DS,NULL,&plan)==-1);
 assert(plan.installed_main_bytes==0u);
 observation=(PspDsMemoryObservation){0};
 observation.slot2_currently_open=1;
 assert(psp_ds_memory_plan(PSP_TARGET_DS,&observation,&plan)==-1);
 observation=(PspDsMemoryObservation){0};
 assert(psp_ds_memory_plan(PSP_TARGET_DS,&observation,&plan)==0);
}
int main(void){
 normal_ds();official_pak_and_reopen();dsi();unsupported_or_unverified();
 puts("PASS: DS 4MiB, Pak 4+8MiB separate, DSi 16MiB/DS-mode 4MiB, checked heap and close-only cold access");
 return 0;
}
