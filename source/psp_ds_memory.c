#include "psp_ds_memory.h"
#include <string.h>
static uint32_t minimum(uint32_t a,uint32_t b){return a<b?a:b;}
int psp_ds_memory_plan(PspTarget model,const PspDsMemoryObservation *obs,
                       PspDsMemoryPlan *out){
 if(out)memset(out,0,sizeof(*out));
 if(!out||!obs||(model!=PSP_TARGET_DS&&model!=PSP_TARGET_DS_SLOT2 &&
                 model!=PSP_TARGET_DSI))return -1;
 const uint32_t main_bytes=model==PSP_TARGET_DSI?
                            PSP_DSI_MAIN_RAM_BYTES:PSP_DS_MAIN_RAM_BYTES;
 if(obs->largest_touched_heap_allocation>main_bytes ||
    obs->slot2_sampled_verified>1 || obs->slot2_single_bank>1 ||
    obs->slot2_currently_open>1)return -1;
 if(model==PSP_TARGET_DS_SLOT2){
  /* Both the SDK's peripheral capabilities AND a successful sparse
   * read/write sweep are required. Sparse samples still do not prove
   * every reported byte is usable or free of address aliasing. */
  if(!obs->slot2_sampled_verified||!obs->slot2_single_bank||
     obs->slot2_reported_bytes<PSP_DS_MEMORY_GRANULE||
     obs->slot2_reported_bytes>PSP_DS_MAX_SLOT2_BYTES||
     (obs->slot2_reported_bytes%PSP_DS_MEMORY_GRANULE))return -2;
 }else{
  if(obs->slot2_reported_bytes||obs->slot2_sampled_verified ||
     obs->slot2_single_bank||obs->slot2_currently_open)return -1;
 }
 PspPlatformBudget b;
 if(psp_platform_budget_choose(model,&b)!=0 ||
    psp_platform_budget_validate(&b)!=0)return -1;
 out->target=model;
 out->installed_main_bytes=main_bytes;
 out->platform_hot_budget_bytes=b.hot_ram_budget;
 out->confirmed_temporary_heap_bytes=obs->largest_touched_heap_allocation;
 out->usable_hot_bytes=minimum(obs->largest_touched_heap_allocation,
                               b.hot_ram_budget)/PSP_DS_MEMORY_GRANULE*
                               PSP_DS_MEMORY_GRANULE;
 out->native_cache_budget_bytes=b.cache_budget;
 out->texture_budget_bytes=b.texture_budget;
 out->io_budget_bytes=b.io_budget;
 out->combined_reported_bytes=main_bytes;
 if(model==PSP_TARGET_DS_SLOT2){
  out->slot2_sampled_verified=1;
  out->slot2_open_for_use=obs->slot2_currently_open;
  out->reported_slot2_bytes=obs->slot2_reported_bytes;
  out->combined_reported_bytes=main_bytes+obs->slot2_reported_bytes;
  /* A conservative cold-data *policy cap*. Do not count all detected
   * Slot-2 bytes as free, nor treat them as normal C heap/CPU memory. */
  out->potential_cold_bytes=minimum(obs->slot2_reported_bytes,
                                    b.cold_data_budget)/
                                    PSP_DS_MEMORY_GRANULE*
                                    PSP_DS_MEMORY_GRANULE;
  if(obs->slot2_currently_open)
   out->active_cold_bytes=out->potential_cold_bytes;
 }
 return 0;
}
