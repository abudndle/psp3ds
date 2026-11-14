#include "psp_imports.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define BASE UINT32_C(0x08800000)
static uint32_t word(const uint8_t *p){
 return (uint32_t)p[0]|(uint32_t)p[1]<<8|
        (uint32_t)p[2]<<16|(uint32_t)p[3]<<24;
}
int main(void){
 uint8_t ram[256];
 memset(ram,0xee,sizeof ram);
 PspCpu cpu;PspHle hle;PspHleService service=(PspHleService)0;
 psp_cpu_init(&cpu,ram,sizeof ram,BASE,BASE+32u);
 psp_hle_init(&hle);
 assert(hle.compiled_sdk_version==0u);
 assert(psp_imports_known_service("SysMemUserForUser",
             UINT32_C(0x358ca1bb),&service)==1);
 assert(service==PSP_HLE_SET_COMPILED_SDK_VERSION);
 assert(psp_imports_known_service("SysMemUserForUser",
             UINT32_C(0x358ca1bc),&service)==0);
 assert(service==(PspHleService)0);
 assert(psp_imports_known_service("OtherLibrary",
             UINT32_C(0x358ca1bb),&service)==0);
 const PspImport target={
  "SysMemUserForUser",UINT32_C(0x358ca1bb),BASE+32u
 };
 assert(psp_imports_link_known(&cpu,&hle,&target,1u)==0);
 assert(hle.count==1u);
 assert(hle.entries[0].service==PSP_HLE_SET_COMPILED_SDK_VERSION);
 assert(word(ram+32u)==UINT32_C(0x03e00008));
 assert((word(ram+36u)&63u)==12u);
 const uint32_t syscall_id=word(ram+36u)>>6;
 assert(syscall_id==hle.entries[0].syscall_id);
 cpu.gpr[31]=BASE+96u;
 cpu.gpr[4]=UINT32_C(0x06060110); /* example PSP 6.60 build SDK version */
 assert(psp_hle_step(&hle,&cpu)==0); /* jr $ra */
 assert(cpu.pc==BASE+36u && cpu.next_pc==BASE+96u);
 assert(psp_hle_step(&hle,&cpu)==1); /* delay-slot syscall */
 assert(cpu.pc==BASE+96u && cpu.gpr[2]==0u);
 assert(hle.compiled_sdk_version==UINT32_C(0x06060110));
 assert(cpu.fault==0);
 /* Repeat with same real binding; it updates state rather than leaking
  * an additional service binding or falsely initializing other sysmem. */
 cpu.pc=BASE+32u;cpu.next_pc=cpu.pc+4u;
 cpu.gpr[4]=UINT32_C(0x06060010);
 assert(psp_hle_step(&hle,&cpu)==0);
 assert(psp_hle_step(&hle,&cpu)==1);
 assert(hle.compiled_sdk_version==UINT32_C(0x06060010));
 assert(hle.count==1u);
 /* All other unknown system-memory NIDs remain rejected without
  * modifying guest RAM or the HLE state. */
 const uint32_t unmodified=word(ram+48u);
 const PspImport unknown={
  "SysMemUserForUser",UINT32_C(0x237dbd4f),BASE+48u
 };
 assert(psp_imports_link_known(&cpu,&hle,&unknown,1u)==-3);
 assert(word(ram+48u)==unmodified && hle.count==1u);
 psp_hle_init(&hle);
 assert(hle.compiled_sdk_version==0u && hle.count==0u);
 puts("PASS: PSP compiled SDK NID 358CA1BB via real syscall delay slot, unsupported imports stay blocked");
 return 0;
}
