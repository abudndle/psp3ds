#include "psp_startup_trace.h"
#include "psp_imports.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#define BASE UINT32_C(0x08800000)
static uint8_t ram[8192],elf[512];
static void w16(uint8_t *p,uint16_t v){p[0]=(uint8_t)v;p[1]=(uint8_t)(v>>8);}
static void w32(uint8_t *p,uint32_t v){
 for(unsigned i=0;i<4;i++)p[i]=(uint8_t)(v>>(8u*i));
}
static void setup(PspCpu *cpu,PspStartupTraceConfig *cfg){
 memset(elf,0,sizeof elf);
 memcpy(elf,"\177ELF",4);elf[4]=1;elf[5]=1;elf[6]=1;
 w16(elf+16,0xffa0u);w16(elf+18,8u);
 w32(elf+20,1u);w32(elf+24,0u);w32(elf+28,52u);
 w16(elf+40,52u);w16(elf+42,32u);w16(elf+44,1u);
 w32(elf+52,1u);w32(elf+56,128u);
 w32(elf+68,256u);w32(elf+72,256u);w32(elf+76,5u);
 memset(ram,0,sizeof ram);
 psp_cpu_init(cpu,ram,sizeof ram,BASE,BASE);
 assert(psp_startup_prepare_cpu(cpu,256u,4096u,0u)==0);
 assert(psp_startup_trace_config_elf(elf,sizeof elf,BASE,
                                    sizeof ram,16u,cfg)==0);
 w32(ram+0u,0x3c040606u);   /* lui a0,0x0606 */
 w32(ram+4u,0x34840110u);  /* ori a0,a0,0x0110 */
 w32(ram+8u,0x0e200008u);  /* jal BASE+32 */
 w32(ram+12u,0u);          /* delay slot */
 w32(ram+16u,0x48000000u);/* unimplemented VFPU, stop */
 w32(ram+512u,0x358ca1bbu);
}
int main(void){
 PspCpu cpu;
 PspStartupTraceConfig cfg;
 PspPrxImportInventory inv={0};
 inv.library_count=1u;inv.function_count=1u;
 strcpy(inv.libraries[0].name,"SysMemUserForUser");
 inv.libraries[0].nid_table=BASE+512u;
 inv.libraries[0].stub_table=BASE+32u;
 inv.libraries[0].function_count=1u;
 PspHle hle;
 psp_hle_init(&hle);
 PspStartupTraceReport report;
 setup(&cpu,&cfg);
 cfg.imports=&inv;
 cfg.hle=&hle;
 assert(psp_startup_trace_run(&cpu,&cfg,&report)==0);
 assert(report.stop==PSP_STARTUP_UNLINKED_IMPORT);
 assert(report.executed==4u && report.stop_pc==BASE+32u);
 assert(report.import_nid==UINT32_C(0x358ca1bb));
 assert(hle.compiled_sdk_version==0u);
 setup(&cpu,&cfg);
 cfg.imports=&inv;cfg.hle=&hle;
 const PspImport known={"SysMemUserForUser",UINT32_C(0x358ca1bb),
                        BASE+32u};
 assert(psp_imports_link_known(&cpu,&hle,&known,1u)==0);
 assert(psp_startup_trace_run(&cpu,&cfg,&report)==0);
 assert(report.stop==PSP_STARTUP_VFPU_REQUIRED);
 assert(report.executed==6u && report.stop_pc==BASE+16u);
 assert(hle.compiled_sdk_version==UINT32_C(0x06060110));
 assert(hle.count==1u);
 assert(report.import_nid==0u);
 /* Unknown NID remains unsupported. Never patch or run unrecognized HLE. */
 setup(&cpu,&cfg);
 cfg.imports=&inv;
 w32(ram+512u,UINT32_C(0xdeadbeef));
 psp_hle_init(&hle);
 cfg.hle=&hle;
 assert(psp_startup_trace_run(&cpu,&cfg,&report)==0);
 assert(report.stop==PSP_STARTUP_UNLINKED_IMPORT);
 assert(report.import_nid==UINT32_C(0xdeadbeef));
 puts("PASS: exact SDK NID permits deterministic next checkpoint; unknown firmware never silently succeeds");
 return 0;
}
