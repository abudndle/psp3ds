#include "psp_perf.h"
#include <string.h>
#include <limits.h>
void psp_perf_init(PspPerf *p){if(p){memset(p,0,sizeof(*p));p->allow_texture_cache=1;}}
int psp_perf_schedule(PspPerf *p,uint64_t deadline,uint32_t token){
 if(!p || !token || p->count>=PSP_PERF_EVENTS || deadline<p->now_us)return -1;
 for(size_t i=0;i<p->count;i++)if(p->pending[i].token==token)return -1;
 size_t i=p->count++;
 while(i && p->pending[i-1].deadline>deadline){p->pending[i]=p->pending[i-1];i--;}
 p->pending[i]=(PspEvent){deadline,token};return 0;
}

/* Preserve chronological order. Equal deadlines retain their prior order,
 * with a rescheduled token inserted after events already at that deadline. */
int psp_perf_cancel_event(PspPerf *p,uint32_t token){
 if(!p || !token)return -1;
 for(size_t i=0;i<p->count;i++){
  if(p->pending[i].token!=token)continue;
  memmove(p->pending+i,p->pending+i+1,(p->count-i-1)*sizeof(PspEvent));
  p->count--;
  return 0;
 }
 return -1;
}
int psp_perf_reschedule_event(PspPerf *p,uint32_t token,uint64_t deadline){
 if(!p || !token || deadline<p->now_us)return -1;
 size_t old=0;
 while(old<p->count && p->pending[old].token!=token)old++;
 if(old==p->count)return -1;
 /* All validations precede mutation; count is unchanged. */
 memmove(p->pending+old,p->pending+old+1,(p->count-old-1)*sizeof(PspEvent));
 p->count--;
 size_t i=p->count++;
 while(i && p->pending[i-1].deadline>deadline){
  p->pending[i]=p->pending[i-1];
  i--;
 }
 p->pending[i]=(PspEvent){deadline,token};
 return 0;
}
void psp_perf_wait_until(PspPerf *p,uint64_t wake){
 if(p && wake>p->now_us){p->thread_state=PSP_THREAD_WAITING;p->wake_us=wake;}
}
uint32_t psp_perf_idle_advance(PspPerf *p,uint64_t until){
 if(!p || until<p->now_us || p->thread_state!=PSP_THREAD_WAITING)return 0;
 uint64_t target=until;
 if(p->wake_us<target)target=p->wake_us;
 if(p->count && p->pending[0].deadline<target)target=p->pending[0].deadline;
 p->now_us=target;
 if(p->wake_us<=target)p->thread_state=PSP_THREAD_RUNNABLE;
 if(p->count && p->pending[0].deadline<=target){
  uint32_t token=p->pending[0].token;
  memmove(p->pending,p->pending+1,(--p->count)*sizeof(PspEvent));
  return token;
 }
 return 0;
}
uint32_t psp_perf_idle_hle(PspPerf *p,PspHle *hle,uint64_t until_us){
 if(!p || !hle)return 0;
 uint64_t before=p->now_us;
 uint32_t event=psp_perf_idle_advance(p,until_us);
 psp_hle_advance_time(hle,p->now_us-before);
 return event;
}
int psp_perf_step(PspPerf *p,PspHle *hle,PspCpu *cpu){
 if(!p || !hle || !cpu)return -2;
 if(p->thread_state==PSP_THREAD_WAITING)return 2;
 return psp_hle_step(hle,cpu);
}
void psp_perf_enable_profiling(PspPerf *p,int enabled){if(p)p->profile_cpu=(uint8_t)(enabled!=0);}
int psp_perf_run_slice(PspPerf *p,PspHle *hle,PspCpu *cpu,unsigned budget,unsigned *executed){
 if(executed)*executed=0;
 if(!p || !hle || !cpu || !executed || !budget || budget>65536u)return -2;
 for(unsigned i=0;i<budget;i++){
  uint32_t pc=cpu->pc;
  int status=psp_perf_step(p,hle,cpu);
  if(status==2)return 2;
  if(status<0)return status;
  (*executed)++;
  if(p->profile_cpu)(void)psp_perf_record_block(p,pc,16);
  if(cpu->pc<pc || status==1)return 0;
 }
 return 0;
}
int psp_perf_request_subsystem(PspPerf *p,PspSubsystem sub){
 if(!p || (unsigned)sub>=PSP_SUBSYSTEM_COUNT)return -1;
 unsigned flag=1u<<(unsigned)sub;
 int first=(p->initialized_mask&flag)==0;
 p->initialized_mask|=flag;return first;
}
int psp_perf_record_block(PspPerf *p,uint32_t pc,uint32_t threshold){
 if(!p || threshold==0)return -1;
 unsigned slot=(pc>>2)%PSP_PERF_HOT_SLOTS;
 if(p->hot_pc[slot]!=pc){p->hot_pc[slot]=pc;p->hot_hits[slot]=0;}
 if(p->hot_hits[slot]!=UINT32_MAX)p->hot_hits[slot]++;
 return p->hot_hits[slot]>=threshold;
}
int psp_perf_texture_seen(PspPerf *p,uint32_t key,uint32_t generation){
 if(!p)return -1;
 if(!p->allow_texture_cache)return 0;
 unsigned slot=key%PSP_PERF_TEXTURE_SLOTS;
 int hit=p->texture_valid[slot] && p->texture_key[slot]==key && p->texture_generation[slot]==generation;
 p->texture_valid[slot]=1;p->texture_key[slot]=key;p->texture_generation[slot]=generation;
 return hit;
}
void psp_perf_invalidate_textures(PspPerf *p){if(p)memset(p->texture_valid,0,sizeof(p->texture_valid));}
void psp_perf_media_changed(PspPerf *p){
 if(!p)return;
 memset(p->sector_valid,0,sizeof(p->sector_valid));
 p->tick=0;
}
int psp_perf_read_sector(PspPerf *p,uint32_t lba,PspSectorRead reader,void *context,uint8_t *out){
 if(!p || !reader || !out)return -1;
 if(p->sector_reader_identity!=reader || p->sector_context_identity!=context){
  psp_perf_media_changed(p);
  p->sector_reader_identity=reader;
  p->sector_context_identity=context;
 }
 for(unsigned i=0;i<PSP_PERF_SECTOR_SLOTS;i++)if(p->sector_valid[i] && p->sector_lba[i]==lba){
  memcpy(out,p->sector_data[i],PSP_PERF_SECTOR_SIZE);p->sector_age[i]=++p->tick;return 1;
 }
 unsigned slot=0;
 for(unsigned i=0;i<PSP_PERF_SECTOR_SLOTS;i++){
  if(!p->sector_valid[i]){slot=i;break;}
  if(p->sector_age[i]<p->sector_age[slot])slot=i;
 }
 uint8_t tmp[PSP_PERF_SECTOR_SIZE];
 if(reader(context,lba,tmp)!=0)return -2;
 memcpy(p->sector_data[slot],tmp,sizeof tmp);memcpy(out,tmp,sizeof tmp);
 p->sector_valid[slot]=1;p->sector_lba[slot]=lba;p->sector_age[slot]=++p->tick;
 return 0;
}
void psp_perf_profile(PspPerf *p,const char *disc_id){
 if(!p)return;
 p->frameskip=0;p->allow_texture_cache=1;
 /* Unknown titles retain safe defaults. No unverified game-specific hacks. */
 (void)disc_id;
}

int psp_perf_set_subsystem_active(PspPerf *p,PspSubsystem sub,int active){
 if(!p || (unsigned)sub>=PSP_SUBSYSTEM_COUNT)return -1;
 unsigned flag=1u<<(unsigned)sub;
 if(active){p->initialized_mask|=flag;p->active_mask|=flag;}
 else p->active_mask&=~flag;
 return 0;
}
int psp_perf_subsystem_active(const PspPerf *p,PspSubsystem sub){
 if(!p || (unsigned)sub>=PSP_SUBSYSTEM_COUNT)return -1;
 return (p->active_mask&(1u<<(unsigned)sub))?1:0;
}
int psp_perf_run_demand(PspPerf *p,PspHle *hle,PspCpu *cpu,
                        unsigned budget,uint64_t deadline,
                        unsigned *executed,uint32_t *event_token){
 if(executed)*executed=0;
 if(event_token)*event_token=0;
 if(!p || !hle || !cpu || !executed || !event_token || budget==0 || budget>65536)
  return -2;
 if(p->thread_state==PSP_THREAD_WAITING){
  if(deadline<p->now_us)return -2;
  uint32_t event=psp_perf_idle_hle(p,hle,deadline);
  if(event){*event_token=event;return 1;}
  return p->thread_state==PSP_THREAD_RUNNABLE?3:2;
 }
 return psp_perf_run_slice(p,hle,cpu,budget,executed);
}
