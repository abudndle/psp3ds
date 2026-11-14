#include "psp_prx_inventory.h"
#include "psp_startup_trace.h"
#include "psp_hle.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define BASE UINT32_C(0x08800000)
static uint8_t ram[4096];
static void w32(unsigned offset,uint32_t value){
 for(unsigned i=0;i<4;i++)ram[offset+i]=(uint8_t)(value>>(i*8u));
}
static uint32_t r32(unsigned offset){
 return (uint32_t)ram[offset]|(uint32_t)ram[offset+1u]<<8u|
        (uint32_t)ram[offset+2u]<<16u|(uint32_t)ram[offset+3u]<<24u;
}
static PspPrxImportInventory sample_inventory(void){
 PspPrxImportInventory p={0};
 p.library_count=2u;
 p.function_count=3u;
 strcpy(p.libraries[0].name,"SysMemUserForUser");
 p.libraries[0].nid_table=BASE+0x300u;
 p.libraries[0].stub_table=BASE+0x100u;
 p.libraries[0].function_count=2u;
 strcpy(p.libraries[1].name,"IoFileMgrForUser");
 p.libraries[1].nid_table=BASE+0x310u;
 p.libraries[1].stub_table=BASE+0x120u;
 p.libraries[1].function_count=1u;
 w32(0x300u,UINT32_C(0x358ca1bb)); /* SDK setter */
 w32(0x304u,UINT32_C(0xdeadbeef)); /* actually unknown */
 w32(0x310u,UINT32_C(0x109f50bc)); /* verified sceIoOpen */
 for(unsigned i=0;i<3u;i++){
  const unsigned off=i<2u?0x100u+i*8u:0x120u;
  w32(off,UINT32_C(0x03e00008)); /* unlinked stub */
  w32(off+4u,0u);
 }
 return p;
}
static void test_link_and_guest(void){
 memset(ram,0,sizeof ram);
 PspPrxImportInventory inv=sample_inventory();
 PspHle hle;
 psp_hle_init(&hle);
 PspCpu cpu;
 psp_cpu_init(&cpu,ram,sizeof ram,BASE,BASE+0x100u);
 PspPrxHleCoverage coverage={0};
 assert(psp_prx_link_verified_imports(&cpu,&hle,&inv,&coverage)==0);
 assert(coverage.implemented==2u && coverage.unsupported==1u);
 assert(hle.count==2u);
 assert(r32(0x100u)==UINT32_C(0x03e00008));
 assert((r32(0x104u)&63u)==12u);
 assert(r32(0x108u)==UINT32_C(0x03e00008));
 assert(r32(0x10cu)==0u); /* unimplemented is NEVER patched */
 assert((r32(0x124u)&63u)==12u);
 PspStartupTraceConfig config={0};
 config.executable[0]=(PspStartupExecutableRange){
   BASE+0x100u,BASE+0x150u};
 config.executable_count=1u;
 config.instruction_cap=2u;
 config.imports=&inv;
 config.hle=&hle;
 cpu.gpr[31]=BASE+0x130u;
 cpu.gpr[4]=UINT32_C(0x06060110);
 PspStartupTraceReport trace;
 assert(psp_startup_trace_run(&cpu,&config,&trace)==0);
 assert(trace.stop==PSP_STARTUP_BUDGET);
 assert(trace.executed==2u);
 assert(trace.handled_hle_traps==1u);
 assert(cpu.pc==BASE+0x130u);
 assert(hle.compiled_sdk_version==UINT32_C(0x06060110));
 assert(cpu.gpr[2]==0u);
 /* The next, unimplemented import is still an explicit blocker. */
 psp_cpu_init(&cpu,ram,sizeof ram,BASE,BASE+0x108u);
 config.instruction_cap=4u;
 assert(psp_startup_trace_run(&cpu,&config,&trace)==0);
 assert(trace.stop==PSP_STARTUP_UNLINKED_IMPORT);
 assert(trace.executed==0u);
 assert(trace.handled_hle_traps==0u);
 assert(trace.import_nid==UINT32_C(0xdeadbeef));
 assert(strcmp(trace.import_library,"SysMemUserForUser")==0);
}
static void test_atomic_validation(void){
 memset(ram,0,sizeof ram);
 PspPrxImportInventory inv=sample_inventory();
 PspHle hle;
 psp_hle_init(&hle);
 PspCpu cpu;
 psp_cpu_init(&cpu,ram,sizeof ram,BASE,BASE);
 PspPrxHleCoverage count={0};
 inv.libraries[1].stub_table=BASE+sizeof ram-4u;
 assert(psp_prx_link_verified_imports(&cpu,&hle,&inv,&count)<0);
 assert(count.implemented==0 && hle.count==0u);
 assert(r32(0x104u)==0u && r32(0x124u)==0u);
 inv=sample_inventory();
 inv.libraries[0].nid_table=BASE+sizeof ram-4u;
 assert(psp_prx_link_verified_imports(&cpu,&hle,&inv,&count)<0);
 assert(hle.count==0u);
 assert(r32(0x104u)==0u && r32(0x124u)==0u);
 inv=sample_inventory();
 assert(psp_prx_link_verified_imports(NULL,&hle,&inv,&count)<0);
 assert(psp_prx_link_verified_imports(&cpu,NULL,&inv,&count)<0);
 assert(psp_prx_link_verified_imports(&cpu,&hle,NULL,&count)<0);
 assert(psp_prx_link_verified_imports(&cpu,&hle,&inv,NULL)<0);
}
int main(void){
 test_link_and_guest();
 test_atomic_validation();
 puts("PASS: 145-style selective PSP HLE binding, real SDK setter and unimplemented import stop");
 return 0;
}
