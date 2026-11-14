#include "psp_memory_guard.h"
#include <assert.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define M (1024u*1024u)
#define K 1024u
typedef struct {
 uint32_t outstanding,allocations,frees;
 uint32_t fail_on_attempt;
} ControlledAllocator;
static void *allocate_test(void *ctx,size_t bytes){
 ControlledAllocator *a=(ControlledAllocator*)ctx;
 a->allocations++;
 if(a->fail_on_attempt==a->allocations)return NULL;
 void *p=malloc(bytes);
 if(p)a->outstanding++;
 return p;
}
static void free_test(void *ctx,void *pointer){
 ControlledAllocator *a=(ControlledAllocator*)ctx;
 assert(a->outstanding>0);
 a->outstanding--;
 a->frees++;
 free(pointer);
}
static void across_models(void){
 for(int i=PSP_TARGET_DS;i<PSP_TARGET_COUNT;i++){
  PspMemoryGuardPolicy policy;
  if(i<=PSP_TARGET_DSI){
   PspDsMemoryObservation obs={0};
   obs.largest_touched_heap_allocation=i==PSP_TARGET_DSI?2u*M:512u*K;
   if(i==PSP_TARGET_DS_SLOT2){
    obs.slot2_reported_bytes=8u*M;
    obs.slot2_sampled_verified=1;
    obs.slot2_single_bank=1;
    obs.slot2_currently_open=0;
   }
   PspDsMemoryPlan plan;
   assert(psp_ds_memory_plan((PspTarget)i,&obs,&plan)==0);
   assert(psp_memory_guard_policy_ds(&plan,&policy)==0);
   assert(policy.total_limit==obs.largest_touched_heap_allocation*3u/4u);
   assert(policy.reserved_outside_guard==
           obs.largest_touched_heap_allocation/4u);
   assert(policy.class_limit[PSP_MEMORY_GUEST]==
          (i==PSP_TARGET_DSI?2u*M:512u*K));
   if(i==PSP_TARGET_DS_SLOT2)
    assert(policy.total_limit<4u*M && plan.active_cold_bytes==0u);
  }else{
   Psp3dsMemoryPlan plan;
   const uint32_t app=i==PSP_TARGET_NEW3DS?124u*M:64u*M;
   const uint32_t free_bytes=i==PSP_TARGET_NEW3DS?110u*M:60u*M;
   assert(psp_3ds_memory_plan((PspTarget)i,app,free_bytes,&plan)==0);
   assert(psp_memory_guard_policy_3ds(&plan,&policy)==0);
   assert(policy.total_limit==plan.permitted_guest_bytes);
   assert(policy.class_limit[PSP_MEMORY_GUEST]==
          (i==PSP_TARGET_NEW3DS?64u*M:32u*M));
   assert(policy.reserved_outside_guard==
          (i==PSP_TARGET_NEW3DS?24u*M:16u*M));
  }
  ControlledAllocator a={0};
  PspMemoryGuard guard={0};
  assert(psp_memory_guard_init(&guard,&policy,allocate_test,free_test,&a)==0);
  for(unsigned cycle=0;cycle<128u;cycle++){
   void *g=NULL,*texture=NULL,*io=NULL;
   assert(psp_memory_guard_acquire(&guard,PSP_MEMORY_GUEST,4096,&g)==0);
   assert(psp_memory_guard_acquire(&guard,PSP_MEMORY_TEXTURE,1024,
                                   &texture)==0);
   assert(psp_memory_guard_acquire(&guard,PSP_MEMORY_IO,128,&io)==0);
   assert(guard.outstanding_allocations==3u);
   assert(guard.allocated_bytes==5248u);
   memset(g,0x33,4096);
   memset(texture,0x22,1024);
   assert(psp_memory_guard_release(&guard,io)==0);
   assert(psp_memory_guard_release(&guard,texture)==0);
   assert(psp_memory_guard_release(&guard,g)==0);
   assert(guard.outstanding_allocations==0u);
   assert(guard.allocated_bytes==0u);
   assert(a.outstanding==0u);
  }
  PspMemoryGuardReport report={0};
  assert(psp_memory_guard_close(&guard,&report)==0);
  assert(report.end_outstanding_bytes==0u &&
         report.reclaimed_allocations==0u &&
         report.reclaimed_bytes==0u);
  assert(report.peak_bytes==5248u);
  assert(report.successful_allocations==128u*3u);
  assert(report.successful_releases==128u*3u);
  assert(a.outstanding==0u && a.allocations==a.frees);
  assert(psp_memory_guard_close(&guard,&report)==-1);
 }
}
static void over_budget_failures(void){
 PspDsMemoryObservation obs={512u*K,0,0,0,0};
 PspDsMemoryPlan plan;
 PspMemoryGuardPolicy policy;
 assert(psp_ds_memory_plan(PSP_TARGET_DS,&obs,&plan)==0);
 assert(psp_memory_guard_policy_ds(&plan,&policy)==0);
 PspMemoryGuard guard={0};
 ControlledAllocator a={0};
 assert(psp_memory_guard_init(&guard,&policy,allocate_test,free_test,&a)==0);
 assert(psp_memory_guard_init(&guard,&policy,allocate_test,free_test,&a)==-1);
 void *guest=(void*)UINTPTR_MAX;
 assert(psp_memory_guard_acquire(&guard,PSP_MEMORY_GUEST,
                                 policy.total_limit+1u,&guest)==-2);
 assert(guest==NULL && a.allocations==0u);
 assert(psp_memory_guard_acquire(&guard,PSP_MEMORY_TEXTURE,
                                 policy.class_limit[PSP_MEMORY_TEXTURE]+1u,
                                 &guest)==-2);
 assert(guest==NULL && a.allocations==0u);
 assert(psp_memory_guard_acquire(&guard,PSP_MEMORY_GUEST,300u*K,&guest)==0);
 assert(psp_memory_guard_acquire(&guard,PSP_MEMORY_GUEST,90u*K,
                                 &(void*){0})==-2);
 assert(guard.allocated_bytes==300u*K);
 assert(psp_memory_guard_release(&guard,guest)==0);
 assert(psp_memory_guard_release(&guard,guest)==-2); /* double free */
 int foreign=0;
 assert(psp_memory_guard_release(&guard,&foreign)==-2);
 assert(psp_memory_guard_acquire(&guard,PSP_MEMORY_CACHE,0,&guest)==-1);
 assert(psp_memory_guard_acquire(&guard,PSP_MEMORY_CACHE,32,NULL)==-1);
 assert(psp_memory_guard_acquire(&guard,(PspMemoryClass)99u,
                                 16u,&guest)==-1);
#if SIZE_MAX > UINT32_MAX
 assert(psp_memory_guard_acquire(&guard,PSP_MEMORY_IO,
                                 (size_t)UINT32_MAX+1u,&guest)==-1);
#endif
 /* Inject an allocator failure, ensure budgets remain unchanged. */
 a.fail_on_attempt=a.allocations+1u;
 assert(psp_memory_guard_acquire(&guard,PSP_MEMORY_IO,512,&guest)==-3);
 assert(!guest && guard.allocated_bytes==0u);
 a.fail_on_attempt=0u;
 PspMemoryGuardReport report;
 assert(psp_memory_guard_close(&guard,&report)==0);
 assert(report.denied_requests>=3u && report.allocator_failures==1u);
 assert(a.outstanding==0u);
}
static void leaked_session_cleanup(void){
 Psp3dsMemoryPlan observed;
 PspMemoryGuardPolicy policy;
 assert(psp_3ds_memory_plan(PSP_TARGET_NEW3DS,124u*M,110u*M,&observed)==0);
 assert(psp_memory_guard_policy_3ds(&observed,&policy)==0);
 ControlledAllocator a={0};
 for(unsigned cycle=0;cycle<96u;cycle++){
  PspMemoryGuard guard={0};
  assert(psp_memory_guard_init(&guard,&policy,allocate_test,free_test,&a)==0);
  void *guest=NULL,*texture=NULL;
  assert(psp_memory_guard_acquire(&guard,PSP_MEMORY_GUEST,4096u,&guest)==0);
  assert(psp_memory_guard_acquire(&guard,PSP_MEMORY_TEXTURE,
                                  3072u,&texture)==0);
  assert(a.outstanding==2u);
  /* Guest can error before explicit free; session end reclaims both.
   * This detects cleanup omissions rather than falsely claiming none. */
  PspMemoryGuardReport report;
  assert(psp_memory_guard_close(&guard,&report)==0);
  assert(report.end_outstanding_bytes==7168u &&
         report.reclaimed_bytes==7168u &&
         report.reclaimed_allocations==2u);
  assert(a.outstanding==0u);
 }
 assert(a.allocations==a.frees);
}
static void slot_limits_and_empty(void){
 PspDsMemoryObservation obs={512u*K,0,0,0,0};
 PspDsMemoryPlan plan;
 PspMemoryGuardPolicy policy;
 assert(psp_ds_memory_plan(PSP_TARGET_DS,&obs,&plan)==0);
 assert(psp_memory_guard_policy_ds(&plan,&policy)==0);
 PspMemoryGuard g={0};
 assert(psp_memory_guard_init(&g,&policy,NULL,NULL,NULL)==0);
 void *ptr[PSP_MEMORY_GUARD_SLOTS]={0};
 for(unsigned i=0;i<PSP_MEMORY_GUARD_SLOTS;i++)
  assert(psp_memory_guard_acquire(&g,PSP_MEMORY_GUEST,1,&ptr[i])==0);
 void *extra=NULL;
 assert(psp_memory_guard_acquire(&g,PSP_MEMORY_GUEST,1,&extra)==-2);
 assert(extra==NULL && g.outstanding_allocations==PSP_MEMORY_GUARD_SLOTS);
 for(unsigned i=0;i<PSP_MEMORY_GUARD_SLOTS;i++)
  assert(psp_memory_guard_release(&g,ptr[i])==0);
 PspMemoryGuardReport report;
 assert(psp_memory_guard_close(&g,&report)==0);
 assert(report.reclaimed_allocations==0u && report.denied_requests==1u);
 obs.largest_touched_heap_allocation=0;
 assert(psp_ds_memory_plan(PSP_TARGET_DS,&obs,&plan)==0);
 assert(psp_memory_guard_policy_ds(&plan,&policy)==-1);
 assert(policy.total_limit==0u);
 Psp3dsMemoryPlan too_low;
 assert(psp_3ds_memory_plan(PSP_TARGET_OLD3DS,64u*M,16u*M,&too_low)==0);
 assert(psp_memory_guard_policy_3ds(&too_low,&policy)==-1);
 assert(policy.total_limit==0u);
 assert(psp_memory_guard_policy_ds(NULL,&policy)==-1);
 assert(psp_memory_guard_policy_ds(&plan,NULL)==-1);
 assert(psp_memory_guard_policy_3ds(NULL,&policy)==-1);
 assert(psp_memory_guard_policy_3ds(&too_low,NULL)==-1);
}
int main(void){
 across_models();over_budget_failures();
 leaked_session_cleanup();slot_limits_and_empty();
 puts("PASS: five-model game quotas, protected headroom, repeated leak-free release and forced cleanup");
 return 0;
}
