#include "psp_thread_cpu.h"
#include <limits.h>
#include <string.h>
 /* An unmapped, unaligned return target reserved for created threads. */
#define PSP_THREAD_RETURN_PC UINT32_MAX
#define PSP_THREAD_K0_SIZE 256u
#define PSP_THREAD_STACK_RESERVE (PSP_THREAD_K0_SIZE + 32u)

static void write_guest_word(uint8_t *address, uint32_t word) {
 for (unsigned i=0;i<4;i++) address[i]=(uint8_t)(word>>(8u*i));
}

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
static int32_t add_context(PspThreadCpu *r, uint8_t priority,
                             uint32_t entry_pc, uint32_t stack_pointer, int dormant) {
 if (!valid_mapping(r) || (entry_pc & 3u) || (stack_pointer & 3u) ||
     entry_pc<r->ram_base || stack_pointer<r->ram_base) return -1;
 uint64_t code_offset=(uint64_t)entry_pc-r->ram_base;
 uint64_t stack_offset=(uint64_t)stack_pointer-r->ram_base;
 if (code_offset>r->ram_size || r->ram_size-code_offset<4 ||
     stack_offset>r->ram_size) return -1;
 int32_t id=dormant ? psp_threads_create_dormant(r->scheduler,priority) :
                     psp_threads_create(r->scheduler,priority);
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
 r->stack_sizes[index]=0;
 r->slot_id[index]=(uint32_t)id;
 return id;
}
int32_t psp_thread_cpu_spawn(PspThreadCpu *r, uint8_t priority,
                             uint32_t entry, uint32_t stack_pointer) {
 return add_context(r, priority, entry, stack_pointer, 0);
}
int psp_thread_cpu_set_stack_arena(PspThreadCpu *r, uint32_t low, uint32_t high) {
 if (!valid_mapping(r) || r->stack_cursor || low < r->ram_base || high <= low ||
     ((low | high) & 15u) || high - r->ram_base > r->ram_size) return -1;
 r->stack_floor=low;
 r->stack_cursor=high;
 return 0;
}
int32_t psp_thread_cpu_create(PspThreadCpu *r, uint8_t priority,
                              uint32_t entry, uint32_t size) {
 if (!valid_mapping(r) || !r->stack_cursor || size < PSP_THREAD_STACK_RESERVE ||
     size > UINT32_MAX-15u) return -1;
 size=(size+15u)&~15u;
 if (size > r->stack_cursor-r->stack_floor) return -1;
 int32_t id=add_context(r,priority,entry,r->stack_cursor,1);
 if (id < 0) return id;
 r->stack_sizes[slot_for(r,(uint32_t)id)]=size;
 r->stack_cursor-=size;
 return id;
}
int psp_thread_cpu_start(PspThreadCpu *r, uint32_t id,
                         uint32_t size, uint32_t args) {
 if (!valid_mapping(r)) return -1;
 int index=slot_for(r,id);
 const PspTask *task=psp_threads_get(r->scheduler,id);
 if (index<0 || !task || task->state!=PSP_TASK_DORMANT ||
     r->stack_sizes[index]<PSP_THREAD_STACK_RESERVE ||
     size>r->stack_sizes[index]-PSP_THREAD_STACK_RESERVE ||
     (size && (args<r->ram_base ||
      (uint64_t)(args-r->ram_base)+size>r->ram_size))) return -1;
 PspCpu *cpu=&r->saved[index];
 uint32_t k0=cpu->gpr[29]-PSP_THREAD_K0_SIZE;
 uint32_t stack_base=cpu->gpr[29]-r->stack_sizes[index];
 uint32_t arg_address=(k0-size)&~15u;
 if (size) memmove(r->ram+(arg_address-r->ram_base),r->ram+(args-r->ram_base),size);
 uint8_t *thread_data=r->ram+(k0-r->ram_base);
 memset(thread_data,0,PSP_THREAD_K0_SIZE);
 write_guest_word(thread_data+0xc0,id);
 write_guest_word(thread_data+0xc8,stack_base);
 write_guest_word(thread_data+0xf8,UINT32_MAX);
 write_guest_word(thread_data+0xfc,UINT32_MAX);
 write_guest_word(r->ram+(stack_base-r->ram_base),id);
 cpu->gpr[26]=k0;
 cpu->gpr[29]=arg_address-16u;
 cpu->gpr[4]=size;
 cpu->gpr[5]=size ? arg_address : 0;
 cpu->gpr[31]=PSP_THREAD_RETURN_PC;
 return psp_threads_start(r->scheduler,id);
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
 r->live->ll_valid=0;
 r->active_id=id;
 return 0;
}
int psp_thread_cpu_step(PspThreadCpu *r) {
 if (!valid_mapping(r)) return -2;
 int32_t selected=psp_threads_pick(r->scheduler);
 if (selected<0) return -2;
 if (!selected) return 2;
 if (psp_thread_cpu_switch(r,(uint32_t)selected)!=0) return -2;
 int status=psp_hle_step(r->hle,r->live);
 int index=slot_for(r,r->active_id);
 if (status>=0 && index>=0 && r->stack_sizes[index] &&
     r->live->pc==PSP_THREAD_RETURN_PC)
  return psp_threads_stop(r->scheduler,r->active_id)==0 ? status : -2;
 return status;
}
