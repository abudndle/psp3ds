#include "psp_hle.h"
#include "psp_io.h"
#include "psp_thread_cpu.h"
#include <string.h>
#include <limits.h>
void psp_hle_init(PspHle *hle) {
 if (hle) memset(hle,0,sizeof(*hle));
}
int psp_hle_bind(PspHle *hle,uint32_t id,PspHleService service) {
 if (!hle || id>0xfffffu || hle->count>=PSP_HLE_MAX_BINDINGS ||
     (service!=PSP_HLE_SYSTEM_TIME_LOW && service!=PSP_HLE_SYSTEM_TIME_WIDE &&
      service!=PSP_HLE_SYSTEM_TIME &&
      service!=PSP_HLE_IO_OPEN && service!=PSP_HLE_IO_READ &&
      service!=PSP_HLE_IO_LSEEK32 && service!=PSP_HLE_IO_CLOSE &&
      service!=PSP_HLE_THREAD_CREATE && service!=PSP_HLE_THREAD_WAIT &&
      service!=PSP_HLE_THREAD_SIGNAL && service!=PSP_HLE_THREAD_EXIT)) return -1;
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
 case PSP_HLE_IO_LSEEK32:
   v0=(uint32_t)(hle->io?psp_io_seek32(hle->io,(int32_t)cpu->gpr[4],
                   (int32_t)cpu->gpr[5],cpu->gpr[6]):-1);
   break;
 case PSP_HLE_IO_CLOSE:
   v0=(uint32_t)(hle->io?psp_io_close(hle->io,(int32_t)cpu->gpr[4]):-1);
   break;
 case PSP_HLE_THREAD_CREATE:
   if(!hle->threads || cpu->gpr[6]>255u) v0=(uint32_t)-1;
   else v0=(uint32_t)psp_thread_cpu_spawn(hle->threads,(uint8_t)cpu->gpr[6],
                                          cpu->gpr[4],cpu->gpr[5]);
   break;
 case PSP_HLE_THREAD_WAIT:
   if(!hle->threads || !hle->threads->scheduler || !hle->threads->active_id)
     v0=(uint32_t)-1;
   else v0=(uint32_t)psp_threads_wait(hle->threads->scheduler,
                         hle->threads->active_id,cpu->gpr[4],
                         (uint64_t)cpu->gpr[5]);
   break;
 case PSP_HLE_THREAD_SIGNAL:
   v0=(uint32_t)(hle->threads && hle->threads->scheduler?
       psp_threads_signal(hle->threads->scheduler,cpu->gpr[4]):-1);
   break;
 case PSP_HLE_THREAD_EXIT:
   v0=(uint32_t)(hle->threads && hle->threads->scheduler &&
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
