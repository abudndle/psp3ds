#include "psp_hle.h"
#include <string.h>
#include <limits.h>
void psp_hle_init(PspHle *hle) {
 if (hle) memset(hle,0,sizeof(*hle));
}
int psp_hle_bind(PspHle *hle,uint32_t id,PspHleService service) {
 if (!hle || id>0xfffffu || hle->count>=PSP_HLE_MAX_BINDINGS ||
     (service!=PSP_HLE_SYSTEM_TIME_LOW && service!=PSP_HLE_SYSTEM_TIME_WIDE &&
      service!=PSP_HLE_SYSTEM_TIME)) return -1;
 for(size_t i=0;i<hle->count;i++) if(hle->entries[i].syscall_id==id) return -1;
 hle->entries[hle->count].syscall_id=id;
 hle->entries[hle->count].service=service;
 hle->count++;
 return 0;
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
