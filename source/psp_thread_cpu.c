#include "psp_thread_cpu.h"
#include <limits.h>
#include <string.h>

static int slot_for(const PspThreadCpu *r, uint32_t id) {
 if (!r || !id) return -1;
 for (int i=0;i<PSP_THREADS_MAX;i++)
  if (r->slot_id[i]==id) return i;
 return -1;
}
static int valid_mapping(const PspThreadCpu *r) {
 return r && r->scheduler && r->live && r->hle && r->ram &&
        r->live->ram==r->ram && r->live->ram_size==r->ram_size &&
        r->live->ram_base==r->ram_base;
}
int psp_thread_cpu_init(PspThreadCpu *r, PspThreads *scheduler,
                        PspCpu *live, PspHle *hle) {
 if (!r || !scheduler || !live || !hle || !live->ram ||
     !live->ram_size) return -1;
 memset(r,0,sizeof(*r));
 r->scheduler=scheduler;
 r->live=live;
 r->hle=hle;
 r->ram=live->ram;
 r->ram_size=live->ram_size;
 r->ram_base=live->ram_base;
 return 0;
}
int32_t psp_thread_cpu_spawn(PspThreadCpu *r, uint8_t priority,
                             uint32_t entry_pc, uint32_t stack_pointer) {
 if (!valid_mapping(r) || (entry_pc & 3u) || (stack_pointer & 3u) ||
     entry_pc<r->ram_base || stack_pointer<r->ram_base) return -1;
 uint64_t code_offset=(uint64_t)entry_pc-r->ram_base;
 uint64_t stack_offset=(uint64_t)stack_pointer-r->ram_base;
 if (code_offset>r->ram_size || r->ram_size-code_offset<4 ||
     stack_offset>r->ram_size) return -1;
 int32_t id=psp_threads_create(r->scheduler,priority);
 if (id<=0) return -1;
 int index=-1;
 for (int i=0;i<PSP_THREADS_MAX;i++) {
  if (r->slot_id[i]==0 || psp_threads_get(r->scheduler,r->slot_id[i])==NULL ||
      psp_threads_get(r->scheduler,r->slot_id[i])->state==PSP_TASK_STOPPED) {
   index=i;break;
  }
 }
 /* A new scheduler task always has a free or retired CPU slot. */
 if (index<0) {psp_threads_stop(r->scheduler,(uint32_t)id);return -1;}
 /* A retired active task must not later flush into its successor's slot. */
 if (r->slot_id[index]==r->active_id) r->active_id=0;
 psp_cpu_init(&r->saved[index],r->ram,r->ram_size,r->ram_base,entry_pc);
 r->saved[index].gpr[29]=stack_pointer;
 r->slot_id[index]=(uint32_t)id;
 return id;
}
int psp_thread_cpu_flush(PspThreadCpu *r) {
 if (!valid_mapping(r) || !r->active_id) return -1;
 int index=slot_for(r,r->active_id);
 if (index<0) return -1;
 r->saved[index]=*r->live;
 return 0;
}
int psp_thread_cpu_switch(PspThreadCpu *r,uint32_t id) {
 if (!valid_mapping(r)) return -1;
 const PspTask *task=psp_threads_get(r->scheduler,id);
 int index=slot_for(r,id);
 if (!task || task->state!=PSP_TASK_RUNNABLE || index<0) return -1;
 if (r->active_id==id) return 0;
 if (r->active_id && psp_thread_cpu_flush(r)!=0) return -1;
 *r->live=r->saved[index];
 r->active_id=id;
 return 0;
}
int psp_thread_cpu_step(PspThreadCpu *r) {
 if (!valid_mapping(r)) return -2;
 int32_t selected=psp_threads_pick(r->scheduler);
 if (selected<0) return -2;
 if (!selected) return 2;
 if (psp_thread_cpu_switch(r,(uint32_t)selected)!=0) return -2;
 return psp_hle_step(r->hle,r->live);
}
