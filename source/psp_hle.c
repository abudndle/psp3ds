#include "psp_hle.h"
#include "psp_io.h"
#include "psp_thread_cpu.h"
#include <string.h>
#include <limits.h>
#include <time.h>
void psp_hle_init(PspHle *hle) {
 if (hle) memset(hle,0,sizeof(*hle));
}
int psp_hle_bind(PspHle *hle,uint32_t id,PspHleService service) {
 if (!hle || id>=PSP_HLE_UNRESOLVED_BASE || hle->count>=PSP_HLE_MAX_BINDINGS ||
     (service!=PSP_HLE_SYSTEM_TIME_LOW && service!=PSP_HLE_SYSTEM_TIME_WIDE &&
      service!=PSP_HLE_SYSTEM_TIME &&
      service!=PSP_HLE_IO_OPEN && service!=PSP_HLE_IO_READ &&
      service!=PSP_HLE_IO_LSEEK32 && service!=PSP_HLE_IO_CLOSE &&
      service!=PSP_HLE_THREAD_CREATE && service!=PSP_HLE_THREAD_WAIT &&
      service!=PSP_HLE_THREAD_SIGNAL && service!=PSP_HLE_THREAD_EXIT &&
      service!=PSP_HLE_SET_COMPILED_SDK && service!=PSP_HLE_GET_COMPILED_SDK &&
      service!=PSP_HLE_SET_COMPILER_VERSION &&
      service!=PSP_HLE_KERNEL_CREATE_THREAD && service!=PSP_HLE_KERNEL_START_THREAD &&
      service!=PSP_HLE_MODULE_ID_BY_ADDRESS &&
      service!=PSP_HLE_INTR_SUSPEND && service!=PSP_HLE_INTR_RESUME && service!=PSP_HLE_PART_ALLOC &&
      service!=PSP_HLE_PART_HEAD && service!=PSP_HLE_PART_FREE &&
      service!=PSP_HLE_STDIN && service!=PSP_HLE_STDOUT && service!=PSP_HLE_STDERR && service!=PSP_HLE_IO_WRITE &&
      service!=PSP_HLE_SET_GPO && service!=PSP_HLE_IMPOSE_LANGUAGE &&
      service!=PSP_HLE_CREATE_CALLBACK && service!=PSP_HLE_REGISTER_EXIT_CALLBACK && service!=PSP_HLE_POWER_REGISTER && service!=PSP_HLE_UMD_REGISTER && service!=PSP_HLE_UMD_CHECK && service!=PSP_HLE_UMD_ACTIVATE && service!=PSP_HLE_UMD_WAIT && service!=PSP_HLE_RTC_CLOCK && service!=PSP_HLE_IO_LSEEK64 &&
      service!=PSP_HLE_CTRL_SET_MODE && service!=PSP_HLE_CTRL_SET_CYCLE)) return -1;
 for(size_t i=0;i<hle->count;i++) if(hle->entries[i].syscall_id==id) return -1;
 hle->entries[hle->count].syscall_id=id;
 hle->entries[hle->count].service=service;
 hle->count++;
 return 0;
}
void psp_hle_attach_io(PspHle *hle,struct PspIo *io) {
 if(hle)hle->io=io;
}
void psp_hle_attach_threads(PspHle *hle,struct PspThreadCpu *threads) {
 if(hle)hle->threads=threads;
}
void psp_hle_advance_time(PspHle *hle,uint64_t delta_us) {
 if(!hle)return;
 if(delta_us>UINT64_MAX-hle->uptime_us)hle->uptime_us=UINT64_MAX;
 else hle->uptime_us+=delta_us;
}
static int guest_ptr(PspCpu *cpu,uint32_t addr,size_t n,size_t *off) {
 if(addr<cpu->ram_base || !cpu->ram)return -1;
 uint64_t offset=(uint64_t)addr-cpu->ram_base;
 if(offset>cpu->ram_size || n>cpu->ram_size-(size_t)offset)return -1;
 *off=(size_t)offset;
 return 0;
}
static void write_u32le(uint8_t *p,uint32_t v) {
 for(unsigned i=0;i<4;i++)p[i]=(uint8_t)(v>>(8*i));
}
#define PART_BLOCK_COUNT 16u
static uint32_t alloc_partition(PspHle *hle, PspCpu *cpu) {
 uint32_t size=cpu->gpr[7], align=cpu->gpr[8], mode=cpu->gpr[6];
 size_t offset;
 if (cpu->gpr[4]!=2 || (mode!=0 && mode!=3) || !size ||
     !cpu->gpr[5] || guest_ptr(cpu,cpu->gpr[5],1,&offset)!=0 ||
     hle->partition_high<=hle->partition_low ||
     guest_ptr(cpu,hle->partition_low,hle->partition_high-hle->partition_low,&offset)!=0)
  return 0x800200d9u;
 if (mode==0) align=256;
 if (!align || (align&(align-1))) return 0x800200d9u;
 if (align<256) align=256;
 unsigned slot=0;
 while (slot<PART_BLOCK_COUNT && hle->partition_blocks[slot].uid) ++slot;
 if (slot==PART_BLOCK_COUNT || hle->partition_next_uid>=0x7fffdfffu)
  return 0x800200d9u;
 uint64_t cursor=hle->partition_low;
 uint64_t length=((uint64_t)size+255u)&~UINT64_C(255);
 while (cursor<hle->partition_high) {
  cursor=(cursor+align-1)&~((uint64_t)align-1);
  if (cursor+length>hle->partition_high) break;
  uint64_t next=cursor;
  for (unsigned i=0;i<PART_BLOCK_COUNT;i++) {
   uint64_t start=hle->partition_blocks[i].address;
   uint64_t end=start+hle->partition_blocks[i].size;
   if (hle->partition_blocks[i].uid && start<cursor+length && end>next)
    next=end;
  }
  if (next==cursor) {
   hle->partition_blocks[slot].address=(uint32_t)cursor;
   hle->partition_blocks[slot].size=(uint32_t)length;
   return hle->partition_blocks[slot].uid=0x2000u+(++hle->partition_next_uid);
  }
  cursor=next;
 }
 return 0x800200d9u;
}

int psp_hle_step(PspHle *hle,PspCpu *cpu) {
 if(!hle || !cpu)return -2;
 int status=psp_cpu_step(cpu);
 if(status!=-3 || ((cpu->cp0_cause>>2)&31u)!=8u)return status;
 const PspHleBinding *binding=NULL;
 if(hle->last_binding_index<hle->count &&
    hle->entries[hle->last_binding_index].syscall_id==cpu->last_trap_code){
   binding=&hle->entries[hle->last_binding_index];
 }else{
   for(size_t i=0;i<hle->count;i++)if(hle->entries[i].syscall_id==cpu->last_trap_code){
     hle->last_binding_index=i;binding=&hle->entries[i];break;
   }
 }
 if(!binding)return -3;
 uint64_t us=hle->uptime_us;
 uint32_t v0=0,v1=cpu->gpr[3];
 switch(binding->service){
 case PSP_HLE_CTRL_SET_MODE:
   if (cpu->gpr[4]>1) v0=0x80000107u;
   else {v0=hle->ctrl_mode;hle->ctrl_mode=cpu->gpr[4];}
   break;
 case PSP_HLE_CTRL_SET_CYCLE:
   if ((cpu->gpr[4] && cpu->gpr[4]<5555u) || cpu->gpr[4]>20000u) v0=0x800001feu;
   else {v0=hle->ctrl_cycle;hle->ctrl_cycle=cpu->gpr[4];}
   break;
 case PSP_HLE_RTC_CLOCK:{
   size_t off;
   v0=0x80010016u;
   if (guest_ptr(cpu,cpu->gpr[4],16,&off)!=0) break;
   time_t now=time(NULL);
   if (now==(time_t)-1) break;
   int64_t seconds=(int64_t)now+(int64_t)(int32_t)cpu->gpr[5]*60;
   time_t adjusted=(time_t)seconds;
   if ((int64_t)adjusted!=seconds) break;
   const struct tm *date=gmtime(&adjusted);
   if (!date || date->tm_year< -1900 || date->tm_year>63635) break;
   const unsigned fields[]={date->tm_year+1900,date->tm_mon+1,date->tm_mday,
                            date->tm_hour,date->tm_min,date->tm_sec};
   for (unsigned i=0;i<6;i++) {
    cpu->ram[off+2*i]=(uint8_t)fields[i];
    cpu->ram[off+2*i+1]=(uint8_t)(fields[i]>>8);
   }
   write_u32le(cpu->ram+off+12,0);
   v0=0;
   break;
 }
 case PSP_HLE_UMD_ACTIVATE:{
   size_t off;
   v0=0x80010016u;
   if (cpu->gpr[4]<1 || cpu->gpr[4]>2 ||
       guest_ptr(cpu,cpu->gpr[5],7,&off)!=0 || memcmp(cpu->ram+off,"disc0:",7)) break;
   v0=0x80010013u;
   if (!hle->io || !hle->io->umd || hle->io->umd->kind==PSP_UMD_NONE) break;
   hle->umd_active_generation=hle->io->umd->generation;
   v0=0;break;
 }
 case PSP_HLE_UMD_WAIT:{
   /* Fixed mounted-media path. A transition requiring a wait is unsupported. */
   unsigned state=1;
   if (hle->io && hle->io->umd && hle->io->umd->kind!=PSP_UMD_NONE)
    state=2u | (hle->umd_active_generation==hle->io->umd->generation ? 0x30u : 0u);
   if (!(cpu->gpr[4] & 0x3bu)) v0=0x80010016u;
   else if (!(state & cpu->gpr[4])) return -3;
   break;
 }
 case PSP_HLE_UMD_CHECK:
   v0=hle->io && hle->io->umd && hle->io->umd->kind!=PSP_UMD_NONE;
   break;
 case PSP_HLE_SET_GPO:hle->guest_debug_output=cpu->gpr[4];break;
 case PSP_HLE_CREATE_CALLBACK:{
   size_t off;
   v0=0x80020190u;
   if (!hle->threads || hle->threads->live!=cpu || !hle->threads->active_id ||
       guest_ptr(cpu,cpu->gpr[4],1,&off)!=0 || (cpu->gpr[5]&3u) ||
       guest_ptr(cpu,cpu->gpr[5],4,&off)!=0) break;
   for (unsigned i=0;i<sizeof hle->callbacks/sizeof hle->callbacks[0];i++) {
    if (hle->callbacks[i].owner) continue;
    hle->callbacks[i].owner=hle->threads->active_id;
    hle->callbacks[i].entry=cpu->gpr[5];
    hle->callbacks[i].argument=cpu->gpr[6];
    v0=0x3000u+i;
    break;
   }
   break;
 }
 case PSP_HLE_POWER_REGISTER:{
   uint32_t slot=cpu->gpr[4], uid=cpu->gpr[5], index=uid-0x3000u;
   const unsigned slots=sizeof hle->power_callbacks/sizeof hle->power_callbacks[0];
   v0=0x80000102u;
   if (slot!=UINT32_MAX && slot>=slots) break;
   v0=0x80000100u;
   if (index>=sizeof hle->callbacks/sizeof hle->callbacks[0] ||
       !hle->callbacks[index].owner) break;
   if (slot==UINT32_MAX) {
    for (slot=0;slot<slots && hle->power_callbacks[slot];slot++) {}
    v0=0x80000022u;
    if (slot==slots) break;
    v0=slot;
   } else {
    v0=0x80000020u;
    if (hle->power_callbacks[slot]) break;
    v0=0;
   }
   hle->power_callbacks[slot]=uid;
   break;
 }
 case PSP_HLE_UMD_REGISTER:
 case PSP_HLE_REGISTER_EXIT_CALLBACK:{
   uint32_t index=cpu->gpr[4]-0x3000u;
   v0=0x800201a1u;
   if (index<sizeof hle->callbacks/sizeof hle->callbacks[0] &&
       hle->callbacks[index].owner) {
    if (binding->service==PSP_HLE_UMD_REGISTER) hle->umd_callback_uid=cpu->gpr[4];
    else hle->exit_callback_uid=cpu->gpr[4];
    v0=0;
   }
   break;
 }
 case PSP_HLE_IMPOSE_LANGUAGE:
   hle->impose_language=cpu->gpr[4];hle->impose_button=cpu->gpr[5];break;
 case PSP_HLE_IO_WRITE:{
   uint32_t fd=cpu->gpr[4],count=cpu->gpr[6];size_t off;
   v0=UINT32_MAX;
   if ((fd==1 || fd==2) && count<=INT32_MAX &&
       guest_ptr(cpu,cpu->gpr[5],count,&off)==0) {
    if (hle->stdio_write && count) hle->stdio_write((int)fd,cpu->ram+off,count);
    v0=count;
   }
   break;
 }
 case PSP_HLE_STDIN:v0=0;break;
 case PSP_HLE_STDOUT:v0=1;break;
 case PSP_HLE_STDERR:v0=2;break;
 case PSP_HLE_PART_ALLOC:v0=alloc_partition(hle,cpu);break;
 case PSP_HLE_PART_HEAD:
 case PSP_HLE_PART_FREE:
   v0=binding->service==PSP_HLE_PART_FREE ? 0x800200cbu : 0;
   for (unsigned i=0;i<PART_BLOCK_COUNT;i++) {
    if (!cpu->gpr[4] || hle->partition_blocks[i].uid!=cpu->gpr[4]) continue;
    if (binding->service==PSP_HLE_PART_FREE) {
     hle->partition_blocks[i].uid=0;
     v0=0;
    } else v0=hle->partition_blocks[i].address;
    break;
   }
   break;
 case PSP_HLE_INTR_SUSPEND:
   v0=(uint32_t)!hle->interrupts_suspended;
   hle->interrupts_suspended=1;
   break;
 case PSP_HLE_INTR_RESUME:
   hle->interrupts_suspended=(cpu->gpr[4] & 1u)==0;
   v0=cpu->gpr[2];
   break;
 case PSP_HLE_MODULE_ID_BY_ADDRESS:
   if (cpu->gpr[4] & 0x80000000u) v0=0x800200d3u;
   else if (cpu->gpr[4]>=hle->module_base &&
            cpu->gpr[4]-hle->module_base<hle->module_size) v0=hle->module_id;
   else v0=0x8002012eu;
   break;
 case PSP_HLE_SET_COMPILED_SDK:hle->compiled_sdk_version=cpu->gpr[4];break;
 case PSP_HLE_GET_COMPILED_SDK:v0=hle->compiled_sdk_version;break;
 case PSP_HLE_SET_COMPILER_VERSION:hle->compiler_version=cpu->gpr[4];break;
 case PSP_HLE_KERNEL_CREATE_THREAD:
   v0=(uint32_t)(hle->threads && hle->threads->live==cpu && cpu->gpr[6]>=8u && cpu->gpr[6]<=119u ?
       psp_thread_cpu_create(hle->threads,(uint8_t)cpu->gpr[6],cpu->gpr[5],cpu->gpr[7]):-1);
   break;
 case PSP_HLE_KERNEL_START_THREAD:
   v0=(uint32_t)(hle->threads && hle->threads->live==cpu ?
       psp_thread_cpu_start(hle->threads,cpu->gpr[4],cpu->gpr[5],cpu->gpr[6]):-1);
   break;
 case PSP_HLE_SYSTEM_TIME_LOW:v0=(uint32_t)us;break;
 case PSP_HLE_SYSTEM_TIME_WIDE:v0=(uint32_t)us;v1=(uint32_t)(us>>32);break;
 case PSP_HLE_SYSTEM_TIME:{
   size_t off;
   if((cpu->gpr[4]&3u) || guest_ptr(cpu,cpu->gpr[4],8,&off)!=0)
     v0=PSP_HLE_ERR_INVALID_ARGUMENT;
   else {
     write_u32le(cpu->ram+off,(uint32_t)us);
     write_u32le(cpu->ram+off+4,(uint32_t)(us>>32));
     v0=0;
   }
   break;
 }
 case PSP_HLE_IO_OPEN:{
   char path[256];size_t i,off;int terminated=0;
   for(i=0;i<sizeof path;i++){
     uint64_t address=(uint64_t)cpu->gpr[4]+i;
     if(address>UINT32_MAX ||
        guest_ptr(cpu,(uint32_t)address,1,&off)!=0)break;
     path[i]=(char)cpu->ram[off];
     if(!path[i]){terminated=1;break;}
   }
   v0=(uint32_t)(!hle->io || !terminated?-1:
      psp_io_open(hle->io,path,cpu->gpr[5]));
   break;
 }
 case PSP_HLE_IO_READ:{
   uint32_t count=cpu->gpr[6];size_t off=0;
   if(!hle->io || count>INT32_MAX ||
      guest_ptr(cpu,cpu->gpr[5],count,&off)!=0)v0=(uint32_t)-1;
   else v0=(uint32_t)psp_io_read(hle->io,(int32_t)cpu->gpr[4],
                                cpu->ram+off,count);
   break;
 }
 case PSP_HLE_IO_LSEEK64:{
   int64_t upper=cpu->gpr[7];
   if (upper>INT32_MAX) upper-=INT64_C(4294967296);
   int64_t displacement=upper*INT64_C(4294967296)+cpu->gpr[6];
   uint64_t result=(uint64_t)(hle->io ?
      psp_io_seek64(hle->io,(int32_t)cpu->gpr[4],displacement,cpu->gpr[8]):-1);
   v0=(uint32_t)result;v1=(uint32_t)(result>>32);
   break;
 }
 case PSP_HLE_IO_LSEEK32:
   v0=(uint32_t)(hle->io?psp_io_seek32(hle->io,(int32_t)cpu->gpr[4],
                   (int32_t)cpu->gpr[5],cpu->gpr[6]):-1);
   break;
 case PSP_HLE_IO_CLOSE:
   v0=(uint32_t)(hle->io?psp_io_close(hle->io,(int32_t)cpu->gpr[4]):-1);
   break;
 case PSP_HLE_THREAD_CREATE:
   if(!hle->threads || hle->threads->live!=cpu || cpu->gpr[6]>255u) v0=(uint32_t)-1;
   else v0=(uint32_t)psp_thread_cpu_spawn(hle->threads,(uint8_t)cpu->gpr[6],
                                          cpu->gpr[4],cpu->gpr[5]);
   break;
 case PSP_HLE_THREAD_WAIT:
   if(!hle->threads || hle->threads->live!=cpu || !hle->threads->scheduler || !hle->threads->active_id)
     v0=(uint32_t)-1;
   else v0=(uint32_t)psp_threads_wait(hle->threads->scheduler,
                         hle->threads->active_id,cpu->gpr[4],
                         (uint64_t)cpu->gpr[5]);
   break;
 case PSP_HLE_THREAD_SIGNAL:
   v0=(uint32_t)(hle->threads && hle->threads->live==cpu && hle->threads->scheduler?
       psp_threads_signal(hle->threads->scheduler,cpu->gpr[4]):-1);
   break;
 case PSP_HLE_THREAD_EXIT:
   v0=(uint32_t)(hle->threads && hle->threads->live==cpu && hle->threads->scheduler &&
       hle->threads->active_id?
       psp_threads_stop(hle->threads->scheduler,hle->threads->active_id):-1);
   break;
 default:return -3;
 }
 cpu->gpr[2]=v0;cpu->gpr[3]=v1;cpu->gpr[0]=0;
 /* Resume at the already resolved branch target for delay-slot syscalls. */
 cpu->pc=cpu->next_pc;
 cpu->next_pc=cpu->pc+4;
 cpu->delay_slot=0;
 cpu->cp0_status &= ~2u;
 cpu->cp0_cause &= ~0x8000007cu;
 cpu->fault=0;
 return 1;
}
