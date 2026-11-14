#include "psp_play_cutbacks.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define M 1048576u
#define K 1024u
static void policy(PspTarget target,PspMemoryGuardPolicy *out){
 if(target<=PSP_TARGET_DSI){
  PspDsMemoryObservation obs={0};
  obs.largest_touched_heap_allocation=target==PSP_TARGET_DSI?
                                   2u*M:512u*K;
  if(target==PSP_TARGET_DS_SLOT2){
   obs.slot2_reported_bytes=8u*M;
   obs.slot2_sampled_verified=1;
   obs.slot2_single_bank=1;
  }
  PspDsMemoryPlan plan;
  assert(psp_ds_memory_plan(target,&obs,&plan)==0);
  assert(psp_memory_guard_policy_ds(&plan,out)==0);
 }else{
  Psp3dsMemoryPlan plan;
  assert(psp_3ds_memory_plan(target,
         target==PSP_TARGET_NEW3DS?124u*M:64u*M,
         target==PSP_TARGET_NEW3DS?110u*M:60u*M,&plan)==0);
  assert(psp_memory_guard_policy_3ds(&plan,out)==0);
 }
}
static void across_hardware(void){
 PspPlayCutbacks all[PSP_TARGET_COUNT];
 for(int i=0;i<PSP_TARGET_COUNT;i++){
  PspPlayCutbacks *p=&all[i];
  assert(psp_play_cutbacks_init(p,(PspTarget)i)==0);
  assert(p->quality==PSP_PLAY_NORMAL);
  assert(p->render_every_n_frames==1u);
  assert(p->cpu_slice_ceiling>0);
  assert(psp_play_cutbacks_should_render(p,0u));
  assert(psp_play_cutbacks_should_render(p,1u));
  PspMemoryGuardPolicy hard;
  policy((PspTarget)i,&hard);
  PspMemoryGuard guard={0};
  assert(psp_memory_guard_init(&guard,&hard,NULL,NULL,NULL)==0);
  assert(psp_play_cutbacks_apply_guard(p,&guard)==0);
  assert(guard.soft_total_limit==hard.total_limit);
  assert(guard.soft_class_limit[PSP_MEMORY_TEXTURE]==
         hard.class_limit[PSP_MEMORY_TEXTURE]);
  const uint32_t frame=p->target_frame_us;
  assert(psp_play_cutbacks_observe(p,frame,0,0,&guard)==0);
  assert(psp_play_cutbacks_observe(p,frame*3u/2u,0,0,&guard)==0);
  assert(psp_play_cutbacks_observe(p,frame*3u/2u,0,0,&guard)==1);
  assert(p->quality==PSP_PLAY_REDUCED);
  assert(p->render_every_n_frames==2u);
  assert(!psp_play_cutbacks_should_render(p,1u));
  assert(psp_play_cutbacks_should_render(p,2u));
  PspRuntimeTuning tune;
  assert(psp_runtime_tuning_choose((PspTarget)i,&tune)==0);
  tune.cpu_slice_current=tune.cpu_slice_max;
  assert(psp_play_cutbacks_apply_tuning(p,&tune)==0);
  assert(tune.cpu_slice_current<=p->cpu_slice_ceiling);
  assert(psp_play_cutbacks_apply_guard(p,&guard)==0);
  assert(guard.soft_total_limit<=hard.total_limit);
  assert(guard.soft_class_limit[PSP_MEMORY_CACHE]<
         hard.class_limit[PSP_MEMORY_CACHE]);
  assert(guard.soft_class_limit[PSP_MEMORY_GUEST]==
         hard.class_limit[PSP_MEMORY_GUEST]);
  const uint32_t normal_triangle=all[i].triangle_limit*2u;
  assert(normal_triangle>0u);
  assert(psp_play_cutbacks_observe(p,frame*3u,0,0,&guard)==1);
  assert(p->quality==PSP_PLAY_EMERGENCY);
  assert(p->render_every_n_frames==3u);
  assert(p->triangle_limit<=normal_triangle/2u);
  assert(psp_play_cutbacks_apply_guard(p,&guard)==0);
  assert(guard.soft_class_limit[PSP_MEMORY_TEXTURE]<
         hard.class_limit[PSP_MEMORY_TEXTURE]);
  assert(psp_play_cutbacks_should_render(p,3u));
  assert(!psp_play_cutbacks_should_render(p,4u));
  /* Return one step only after eight healthy observation frames. */
  for(unsigned j=0;j<7;j++)
   assert(psp_play_cutbacks_observe(p,frame,0,0,&guard)==0);
  assert(p->quality==PSP_PLAY_EMERGENCY);
  assert(psp_play_cutbacks_observe(p,frame,0,0,&guard)==1);
  assert(p->quality==PSP_PLAY_REDUCED);
  for(unsigned j=0;j<8;j++)
   assert(psp_play_cutbacks_observe(p,frame,0,0,&guard)==(j==7?1:0));
  assert(p->quality==PSP_PLAY_NORMAL);
  assert(psp_play_cutbacks_apply_guard(p,&guard)==0);
  assert(guard.soft_total_limit==hard.total_limit);
  PspMemoryGuardReport report;
  assert(psp_memory_guard_close(&guard,&report)==0);
  assert(report.reclaimed_allocations==0);
 }
 assert(all[PSP_TARGET_DS].cpu_slice_ceiling==
        all[PSP_TARGET_DS_SLOT2].cpu_slice_ceiling);
 assert(all[PSP_TARGET_DS].triangle_limit==
        all[PSP_TARGET_DS_SLOT2].triangle_limit);
 assert(all[PSP_TARGET_NEW3DS].triangle_limit>
        all[PSP_TARGET_OLD3DS].triangle_limit);
 assert(all[PSP_TARGET_DSI].triangle_limit>
        all[PSP_TARGET_DS].triangle_limit);
}
static void memory_pressure_and_limits(void){
 PspMemoryGuardPolicy hard;
 policy(PSP_TARGET_OLD3DS,&hard);
 PspMemoryGuard guard={0};
 PspPlayCutbacks p;
 assert(psp_memory_guard_init(&guard,&hard,NULL,NULL,NULL)==0);
 assert(psp_play_cutbacks_init(&p,PSP_TARGET_OLD3DS)==0);
 void *guest=NULL,*texture=NULL;
 assert(psp_memory_guard_acquire(&guard,PSP_MEMORY_GUEST,4u*M,&guest)==0);
 assert(psp_memory_guard_acquire(&guard,PSP_MEMORY_TEXTURE,
                                 hard.class_limit[PSP_MEMORY_TEXTURE],&texture)==0);
 /* Emergency cutbacks never silently free guest RAM, render buffers or
  * change game CPU registers. They cap only subsequent allocations. */
 assert(psp_play_cutbacks_observe(&p,15000,8u*M,16u*M,&guard)==1);
 assert(p.quality==PSP_PLAY_EMERGENCY);
 assert(psp_play_cutbacks_apply_guard(&p,&guard)==0);
 assert(guard.allocated_bytes==4u*M+
         hard.class_limit[PSP_MEMORY_TEXTURE]);
 void *extra=(void*)UINTPTR_MAX;
 assert(psp_memory_guard_acquire(&guard,PSP_MEMORY_TEXTURE,16u,&extra)==-2);
 assert(extra==NULL);
 assert(psp_memory_guard_release(&guard,texture)==0);
 assert(psp_memory_guard_acquire(&guard,PSP_MEMORY_TEXTURE,
         guard.soft_class_limit[PSP_MEMORY_TEXTURE],&texture)==0);
 assert(psp_memory_guard_release(&guard,texture)==0);
 assert(psp_memory_guard_release(&guard,guest)==0);
 /* A failed allocation is an emergency even when speed is normal. */
 assert(psp_play_cutbacks_init(&p,PSP_TARGET_OLD3DS)==0);
 assert(psp_play_cutbacks_observe(&p,15000,0,0,&guard)==1);
 assert(p.quality==PSP_PLAY_EMERGENCY);
 assert(psp_play_cutbacks_apply_guard(&p,&guard)==0);
 assert(psp_memory_guard_close(&guard,NULL)==0);
}
static void defensive_inputs(void){
 PspPlayCutbacks p;
 assert(psp_play_cutbacks_init(&p,PSP_TARGET_COUNT)==-1);
 assert(p.target_frame_us==0u);
 assert(psp_play_cutbacks_init(NULL,PSP_TARGET_DS)==-1);
 assert(psp_play_cutbacks_init(&p,PSP_TARGET_DS)==0);
 PspPlayCutbacks before=p;
 assert(psp_play_cutbacks_observe(&p,100,100,0,NULL)==-1);
 assert(psp_play_cutbacks_observe(&p,100,0,100,NULL)==-1);
 assert(memcmp(&p,&before,sizeof p)==0);
 assert(psp_play_cutbacks_observe(NULL,100,0,0,NULL)==-1);
 assert(psp_play_cutbacks_should_render(NULL,0)==0);
 assert(strcmp(psp_play_quality_name(PSP_PLAY_EMERGENCY),"emergency")==0);
 assert(strcmp(psp_play_quality_name((PspPlayQuality)255u),"invalid")==0);
}
int main(void){
 across_hardware();
 memory_pressure_and_limits();
 defensive_inputs();
 puts("PASS: all five handheld cutbacks, feedback hysteresis, cache throttling, no guest memory eviction");
 return 0;
}
