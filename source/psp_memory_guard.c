#include "psp_memory_guard.h"
#include <stdlib.h>
#include <string.h>
#define GUARD_INITIALIZED 0x50535047u
static uint32_t smaller(uint32_t a,uint32_t b){return a<b?a:b;}
static void *standard_allocate(void *opaque,size_t size){
 (void)opaque;
 return malloc(size);
}
static void standard_release(void *opaque,void *ptr){
 (void)opaque;
 free(ptr);
}
static int fill_policy(PspTarget target,uint32_t total,uint32_t observed,
                       uint32_t reserve,uint32_t guest,
                       PspMemoryGuardPolicy *out){
 PspPlatformBudget base;
 if(!out || psp_platform_budget_choose(target,&base)!=0 ||
    psp_platform_budget_validate(&base)!=0)return -1;
 if(!total || !guest || reserve>observed || total>observed-reserve)
  return -1;
 out->target=target;
 out->total_limit=total;
 out->observed_available_bytes=observed;
 out->reserved_outside_guard=reserve;
 out->class_limit[PSP_MEMORY_GUEST]=guest;
 out->class_limit[PSP_MEMORY_TEXTURE]=base.texture_budget;
 out->class_limit[PSP_MEMORY_CACHE]=base.cache_budget;
 out->class_limit[PSP_MEMORY_IO]=base.io_budget;
 return 0;
}
int psp_memory_guard_policy_ds(const PspDsMemoryPlan *p,
                               PspMemoryGuardPolicy *out){
 if(out)memset(out,0,sizeof(*out));
 if(!out||!p||(p->target!=PSP_TARGET_DS&&
    p->target!=PSP_TARGET_DS_SLOT2&&p->target!=PSP_TARGET_DSI))
  return -1;
 const uint32_t main_ram=p->target==PSP_TARGET_DSI?
                         PSP_DSI_MAIN_RAM_BYTES:PSP_DS_MAIN_RAM_BYTES;
 PspPlatformBudget base;
 if(psp_platform_budget_choose(p->target,&base)!=0 ||
    psp_platform_budget_validate(&base)!=0||
    p->installed_main_bytes!=main_ram ||
    p->confirmed_temporary_heap_bytes>main_ram ||
    p->usable_hot_bytes>base.hot_ram_budget ||
    p->usable_hot_bytes>p->confirmed_temporary_heap_bytes ||
    (p->target==PSP_TARGET_DS_SLOT2 ?
       (p->active_cold_bytes>p->potential_cold_bytes ||
        (!p->slot2_open_for_use && p->active_cold_bytes)!=0) :
       (p->reported_slot2_bytes!=0||p->active_cold_bytes!=0)))
  return -1;
 /* Keep at least 1/4 of the PROBED scratch heap uncommitted to this
  * allocator. The console/runtime can require much more RAM outside it.
  * No reserve claim is based on physical main-RAM totals. */
 const uint32_t total=(p->confirmed_temporary_heap_bytes/4u)*3u;
 const uint32_t aligned=total/PSP_DS_MEMORY_GRANULE*PSP_DS_MEMORY_GRANULE;
 if(!aligned || !p->usable_hot_bytes)return -1;
 return fill_policy(p->target,aligned,
                    p->confirmed_temporary_heap_bytes,
                    p->confirmed_temporary_heap_bytes-aligned,
                    p->usable_hot_bytes,out);
}
int psp_memory_guard_policy_3ds(const Psp3dsMemoryPlan *p,
                                PspMemoryGuardPolicy *out){
 if(out)memset(out,0,sizeof(*out));
 if(!p||!out || (p->target!=PSP_TARGET_OLD3DS &&
                 p->target!=PSP_TARGET_NEW3DS))return -1;
 const int is_new=p->target==PSP_TARGET_NEW3DS;
 const uint32_t physical=is_new?PSP_3DS_NEW_PHYSICAL:PSP_3DS_OLD_PHYSICAL;
 const uint32_t reserve=is_new?PSP_3DS_NEW_HEADROOM:PSP_3DS_OLD_HEADROOM;
 const uint32_t cap=is_new?PSP_3DS_NEW_PSP_GUEST_CAP:
                           PSP_3DS_OLD_PSP_GUEST_CAP;
 if(p->installed_ram_bytes!=physical ||
    p->preserve_for_system_bytes!=reserve ||
    p->observed_application_bytes>physical ||
    p->observed_free_bytes>p->observed_application_bytes ||
    p->observed_free_bytes<reserve ||
    p->permitted_guest_bytes!=p->observed_free_bytes-reserve ||
    !p->recommended_guest_bytes ||
    p->recommended_guest_bytes>cap ||
    p->recommended_guest_bytes>p->permitted_guest_bytes)return -1;
 /* This aggregate ceiling includes guest RAM and resource caches.
  * It is limited by the live free-APPLICATION-memory report minus
  * model-specific system/audio/rendering headroom. */
 return fill_policy(p->target,p->permitted_guest_bytes,
                    p->observed_free_bytes,reserve,
                    p->recommended_guest_bytes,out);
}
int psp_memory_guard_init(PspMemoryGuard *g,
                          const PspMemoryGuardPolicy *policy,
                          PspGuardMalloc allocate,PspGuardFree release,
                          void *opaque){
 if(!g||!policy||g->initialized==GUARD_INITIALIZED ||
    (!!allocate!=!!release) || !policy->total_limit ||
    (unsigned)policy->target>=PSP_TARGET_COUNT ||
    !policy->observed_available_bytes ||
    policy->reserved_outside_guard>=policy->observed_available_bytes ||
    policy->total_limit>policy->observed_available_bytes-
                        policy->reserved_outside_guard)
  return -1;
 PspPlatformBudget base;
 if(psp_platform_budget_choose(policy->target,&base)!=0 ||
    psp_platform_budget_validate(&base)!=0)return -1;
 const int is_ds=policy->target<=PSP_TARGET_DSI;
 const uint32_t max_physical=policy->target==PSP_TARGET_DSI?
         PSP_DSI_MAIN_RAM_BYTES:policy->target<=PSP_TARGET_DS_SLOT2?
         PSP_DS_MAIN_RAM_BYTES:policy->target==PSP_TARGET_NEW3DS?
         PSP_3DS_NEW_PHYSICAL:PSP_3DS_OLD_PHYSICAL;
 if(policy->observed_available_bytes>max_physical)return -1;
 const uint32_t required_reserve=is_ds?1u:
        policy->target==PSP_TARGET_NEW3DS?
        PSP_3DS_NEW_HEADROOM:PSP_3DS_OLD_HEADROOM;
 if(policy->reserved_outside_guard<required_reserve ||
    (is_ds && policy->total_limit>
                policy->observed_available_bytes-
                policy->observed_available_bytes/4u))
  return -1;
 const uint32_t max_guest=is_ds?base.hot_ram_budget:
      policy->target==PSP_TARGET_NEW3DS?
      PSP_3DS_NEW_PSP_GUEST_CAP:PSP_3DS_OLD_PSP_GUEST_CAP;
 if(!policy->class_limit[PSP_MEMORY_GUEST] ||
    policy->class_limit[PSP_MEMORY_GUEST]>max_guest ||
    !policy->class_limit[PSP_MEMORY_TEXTURE] ||
    policy->class_limit[PSP_MEMORY_TEXTURE]>base.texture_budget ||
    !policy->class_limit[PSP_MEMORY_CACHE] ||
    policy->class_limit[PSP_MEMORY_CACHE]>base.cache_budget ||
    !policy->class_limit[PSP_MEMORY_IO] ||
    policy->class_limit[PSP_MEMORY_IO]>base.io_budget)return -1;
 memset(g,0,sizeof(*g));
 g->policy=*policy;
 g->allocate=allocate?allocate:standard_allocate;
 g->deallocate=release?release:standard_release;
 g->opaque=opaque;
 g->initialized=GUARD_INITIALIZED;
 return 0;
}
int psp_memory_guard_acquire(PspMemoryGuard *g,PspMemoryClass cls,
                             size_t bytes,void **result){
 if(result)*result=NULL;
 if(!g||!result||g->initialized!=GUARD_INITIALIZED ||
    (unsigned)cls>=PSP_MEMORY_GUARD_CLASSES ||
    !bytes || bytes>UINT32_MAX)return -1;
 const uint32_t request=(uint32_t)bytes;
 if(request>g->policy.total_limit-g->allocated_bytes ||
    request>g->policy.class_limit[cls]-g->per_class[cls]){
  g->denied_requests++;return -2;
 }
 unsigned slot=PSP_MEMORY_GUARD_SLOTS;
 for(unsigned i=0;i<PSP_MEMORY_GUARD_SLOTS;i++)
  if(!g->tracked[i].pointer){slot=i;break;}
 if(slot==PSP_MEMORY_GUARD_SLOTS){
  g->denied_requests++;return -2;
 }
 void *memory=g->allocate(g->opaque,bytes);
 if(!memory){g->allocator_failures++;return -3;}
 /* A broken custom allocator must never corrupt an existing ownership
  * record. Do not free a duplicate pointer: it is already owned. */
 for(unsigned i=0;i<PSP_MEMORY_GUARD_SLOTS;i++)
  if(g->tracked[i].pointer==memory)return -4;
 g->tracked[slot]=(PspGuardAllocation){memory,request,cls};
 g->allocated_bytes+=request;
 g->per_class[cls]+=request;
 if(g->allocated_bytes>g->peak_bytes)g->peak_bytes=g->allocated_bytes;
 if(g->per_class[cls]>g->peak_class[cls])
  g->peak_class[cls]=g->per_class[cls];
 g->outstanding_allocations++;
 g->successful_allocations++;
 *result=memory;
 return 0;
}
int psp_memory_guard_release(PspMemoryGuard *g,void *memory){
 if(!g || g->initialized!=GUARD_INITIALIZED || !memory)return -1;
 for(unsigned i=0;i<PSP_MEMORY_GUARD_SLOTS;i++){
  PspGuardAllocation *e=&g->tracked[i];
  if(e->pointer!=memory)continue;
  g->allocated_bytes-=e->bytes;
  g->per_class[e->category]-=e->bytes;
  g->outstanding_allocations--;
  g->successful_releases++;
  g->deallocate(g->opaque,memory);
  *e=(PspGuardAllocation){0};
  return 0;
 }
 return -2; /* untracked / already freed; DO NOT free twice */
}
int psp_memory_guard_close(PspMemoryGuard *g,PspMemoryGuardReport *out){
 if(out)memset(out,0,sizeof(*out));
 if(!g || g->initialized!=GUARD_INITIALIZED)return -1;
 PspMemoryGuardReport result={0};
 result.peak_bytes=g->peak_bytes;
 result.end_outstanding_bytes=g->allocated_bytes;
 result.denied_requests=g->denied_requests;
 result.allocator_failures=g->allocator_failures;
 result.successful_allocations=g->successful_allocations;
 result.successful_releases=g->successful_releases;
 for(unsigned i=0;i<PSP_MEMORY_GUARD_SLOTS;i++){
  PspGuardAllocation *e=&g->tracked[i];
  if(!e->pointer)continue;
  result.reclaimed_bytes+=e->bytes;
  result.reclaimed_allocations++;
  g->deallocate(g->opaque,e->pointer);
  *e=(PspGuardAllocation){0};
 }
 memset(g,0,sizeof(*g));
 if(out)*out=result;
 return 0;
}
