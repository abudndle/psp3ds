#include "psp_play_cutbacks.h"
#include <string.h>
#include <stdint.h>
static uint32_t scale(uint32_t value,unsigned pct){
 return (uint32_t)(((uint64_t)value*pct)/100u);
}
static int fill_quality(PspPlayCutbacks *p){
 PspRuntimeTuning tuning;
 PspPlatformBudget budget;
 if(psp_runtime_tuning_choose(p->target,&tuning)!=0 ||
    psp_platform_budget_choose(p->target,&budget)!=0)return -1;
 const unsigned pct=p->quality==PSP_PLAY_NORMAL?100u:
                    p->quality==PSP_PLAY_REDUCED?75u:50u;
 const unsigned tri_pct=p->quality==PSP_PLAY_NORMAL?100u:
                        p->quality==PSP_PLAY_REDUCED?50u:25u;
 p->triangle_limit=(uint16_t)scale(tuning.triangle_demo_budget,tri_pct);
 if(!p->triangle_limit)p->triangle_limit=1;
 p->render_every_n_frames=p->quality==PSP_PLAY_NORMAL?1u:
                          p->quality==PSP_PLAY_REDUCED?2u:3u;
 p->texture_soft_bytes=scale(budget.texture_budget,pct);
 p->cache_soft_bytes=scale(budget.cache_budget,pct);
 p->io_soft_bytes=scale(budget.io_budget,pct);
 p->cpu_slice_ceiling=scale(tuning.cpu_slice_max,tri_pct);
 if(p->cpu_slice_ceiling<tuning.cpu_slice_initial/2u)
  p->cpu_slice_ceiling=tuning.cpu_slice_initial/2u;
 return 0;
}
int psp_play_cutbacks_init(PspPlayCutbacks *out,PspTarget target){
 if(out)memset(out,0,sizeof(*out));
 if(!out||(unsigned)target>=PSP_TARGET_COUNT)return -1;
 out->target=target;
 out->quality=PSP_PLAY_NORMAL;
 /* DS/DS+Pak and DSi have identical native display capabilities;
  * New3DS may target shorter presentation intervals, but not faster
  * PSP virtual time. Never infer available GPU RAM from installed RAM. */
 out->target_frame_us=target==PSP_TARGET_NEW3DS?16667u:33333u;
 return fill_quality(out);
}
int psp_play_cutbacks_observe(PspPlayCutbacks *p,uint32_t frame_us,
                              uint32_t free_bytes,uint32_t reserve,
                              const PspMemoryGuard *guard){
 if(!p||(unsigned)p->target>=PSP_TARGET_COUNT ||
    p->quality>PSP_PLAY_EMERGENCY ||!p->target_frame_us ||
    (free_bytes && !reserve)||(!free_bytes&&reserve) ||
    (guard && (guard->initialized==0 ||guard->policy.target!=p->target)))
  return -1;
 PspPlayQuality pressure=PSP_PLAY_NORMAL;
 if(frame_us){
  if(frame_us>(uint64_t)p->target_frame_us*2u)
   pressure=PSP_PLAY_EMERGENCY;
  else if((uint64_t)frame_us*5u>(uint64_t)p->target_frame_us*6u)
   pressure=PSP_PLAY_REDUCED;
 }
 if(free_bytes){
  if(free_bytes<=reserve)pressure=PSP_PLAY_EMERGENCY;
  else if((uint64_t)free_bytes*2u<=(uint64_t)reserve*3u &&
          pressure<PSP_PLAY_REDUCED)pressure=PSP_PLAY_REDUCED;
 }
 if(guard){
  if(guard->denied_requests!=p->previous_denials ||
     guard->allocator_failures!=p->previous_alloc_failures)
   pressure=PSP_PLAY_EMERGENCY;
  p->previous_denials=guard->denied_requests;
  p->previous_alloc_failures=guard->allocator_failures;
  if(guard->policy.total_limit){
   const uint64_t ratio=(uint64_t)guard->allocated_bytes*100u;
   if(ratio>=(uint64_t)guard->policy.total_limit*95u)
    pressure=PSP_PLAY_EMERGENCY;
   else if(ratio>=(uint64_t)guard->policy.total_limit*80u &&
           pressure<PSP_PLAY_REDUCED)
    pressure=PSP_PLAY_REDUCED;
  }
 }
 PspPlayQuality next=p->quality;
 if(pressure>p->quality){
  p->recovery_streak=0;
  if(pressure==PSP_PLAY_EMERGENCY){
   next=PSP_PLAY_EMERGENCY;p->high_pressure_streak=0;
  }else if(++p->high_pressure_streak>=2u){
   next=pressure;p->high_pressure_streak=0;
  }
 }else if(pressure<p->quality){
  p->high_pressure_streak=0;
  if(++p->recovery_streak>=8u){
   next=(PspPlayQuality)(p->quality-1u);
   p->recovery_streak=0;
  }
 }else{
  p->high_pressure_streak=0;
  p->recovery_streak=0;
 }
 if(next==p->quality)return 0;
 p->quality=next;
 return fill_quality(p)==0?1:-1;
}
int psp_play_cutbacks_apply_guard(const PspPlayCutbacks *p,
                                   PspMemoryGuard *guard){
 if(!p||!guard||guard->policy.target!=p->target ||
    (unsigned)p->target>=PSP_TARGET_COUNT)return -1;
 const uint32_t soft[PSP_MEMORY_GUARD_CLASSES]={
  guard->policy.class_limit[PSP_MEMORY_GUEST],
  p->texture_soft_bytes, p->cache_soft_bytes,p->io_soft_bytes
 };
 const unsigned pct=p->quality==PSP_PLAY_NORMAL?100u:
                    p->quality==PSP_PLAY_REDUCED?90u:75u;
 return psp_memory_guard_set_soft_limits(guard,
                       scale(guard->policy.total_limit,pct),soft);
}
int psp_play_cutbacks_apply_tuning(const PspPlayCutbacks *p,
                                    PspRuntimeTuning *tuning){
 if(!p||!tuning||p->target!=tuning->target||
    !p->cpu_slice_ceiling||!tuning->cpu_slice_current)return -1;
 if(tuning->cpu_slice_current>p->cpu_slice_ceiling)
  tuning->cpu_slice_current=p->cpu_slice_ceiling;
 return 0;
}
int psp_play_cutbacks_should_render(const PspPlayCutbacks *p,uint32_t frame){
 if(!p||!p->render_every_n_frames)return 0;
 return frame%p->render_every_n_frames==0;
}
const char *psp_play_quality_name(PspPlayQuality tier){
 switch(tier){
  case PSP_PLAY_NORMAL:return "normal";
  case PSP_PLAY_REDUCED:return "reduced";
  case PSP_PLAY_EMERGENCY:return "emergency";
  default:return "invalid";
 }
}
