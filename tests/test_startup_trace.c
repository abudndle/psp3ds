#include "psp_startup_trace.h"
#include "psp_guarded_prx.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define BASE UINT32_C(0x08800000)
static uint8_t ram[8192];
static uint8_t elf[512];
static void w16(uint8_t *p,uint16_t v){
 p[0]=(uint8_t)v;p[1]=(uint8_t)(v>>8);
}
static void w32(uint8_t *p,uint32_t v){
 for(unsigned i=0;i<4;i++)p[i]=(uint8_t)(v>>(8u*i));
}
static void fixture(void){
 memset(elf,0,sizeof elf);
 memcpy(elf,"\177ELF",4);
 elf[4]=1;elf[5]=1;elf[6]=1;
 w16(elf+16,0xffa0u);w16(elf+18,8u);
 w32(elf+20,1u);w32(elf+24,0u);w32(elf+28,52u);
 w16(elf+40,52u);w16(elf+42,32u);w16(elf+44,1u);
 w32(elf+52,1u);
 w32(elf+56,128u);
 w32(elf+60,0u);
 w32(elf+68,256u);
 w32(elf+72,256u);
 w32(elf+76,5u); /* file-backed, executable PT_LOAD */
}
static void reset(PspCpu *cpu){
 memset(ram,0,sizeof ram);
 psp_cpu_init(cpu,ram,sizeof ram,BASE,BASE);
 assert(psp_startup_prepare_cpu(cpu,256u,4096u,BASE+128u)==0);
 assert(cpu->gpr[29]==BASE+sizeof ram-16u);
 assert(cpu->gpr[28]==BASE+128u);
 assert(cpu->gpr[31]==0u);
}
static PspStartupTraceConfig config(void){
 PspStartupTraceConfig opts;
 assert(psp_startup_trace_config_elf(elf,sizeof elf,BASE,sizeof ram,
                                    32u,&opts)==0);
 assert(opts.executable_count==1u);
 assert(opts.executable[0].first==BASE);
 assert(opts.executable[0].one_past_last==BASE+256u);
 return opts;
}
static void known_import(void){
 PspCpu cpu;
 PspStartupTraceConfig opts=config();
 PspPrxImportInventory inv={0};
 inv.library_count=1u;inv.function_count=1u;
 strcpy(inv.libraries[0].name,"ThreadManForUser");
 inv.libraries[0].function_count=1u;
 inv.libraries[0].stub_table=BASE+32u;
 inv.libraries[0].nid_table=BASE+512u;
 opts.imports=&inv;
 PspStartupTraceReport first={0};
 for(int repeat=0;repeat<2;repeat++){
  reset(&cpu);
  w32(cpu.ram,0x27bdffe0u);    /* addiu $sp,$sp,-32 */
  w32(cpu.ram+4u,0xafbf001cu); /* sw $ra,28($sp) */
  w32(cpu.ram+8u,0x25080001u);/* addiu $t0,$t0,1 */
  w32(cpu.ram+12u,0x0c200008u);/* jal base+32 */
  w32(cpu.ram+16u,0u);         /* delay-slot NOP */
  w32(cpu.ram+32u,0x03e00008u);/* jr $ra import stub placeholder */
  w32(cpu.ram+512u,0xdeadbeefu);
  PspStartupTraceReport report;
  assert(psp_startup_trace_run(&cpu,&opts,&report)==0);
  assert(report.stop==PSP_STARTUP_UNLINKED_IMPORT);
  assert(report.executed==5u);
  assert(report.stop_pc==BASE+32u);
  assert(report.import_nid==UINT32_C(0xdeadbeef));
  assert(strcmp(report.import_library,"ThreadManForUser")==0);
  assert(report.import_known_hle==0u);
  assert(cpu.gpr[8]==1u);
  assert(cpu.gpr[29]==BASE+sizeof ram-48u);
  if(repeat==0)first=report;
  else assert(first.pc_fingerprint==report.pc_fingerprint &&
              first.stop_pc==report.stop_pc &&
              first.executed==report.executed);
 }
 strcpy(inv.libraries[0].name,"IoFileMgrForUser");
 reset(&cpu);
 w32(cpu.ram+512u,0x109f50bcu); /* existing verified sceIoOpen NID */
 cpu.pc=BASE+32u;cpu.next_pc=cpu.pc+4u;
 PspStartupTraceReport report;
 assert(psp_startup_trace_run(&cpu,&opts,&report)==0);
 assert(report.stop==PSP_STARTUP_UNLINKED_IMPORT &&
        report.import_known_hle==1u);
 /* A recognized NID is not silently treated as successful or linked. */
 assert(report.executed==0u && cpu.pc==BASE+32u);
}
static void first_missing_cpu_behavior(void){
 PspCpu cpu;
 PspStartupTraceConfig opts=config();
 PspStartupTraceReport report;
 reset(&cpu);
 w32(ram,0x48000000u); /* COP2/VFPU major opcode */
 assert(psp_startup_trace_run(&cpu,&opts,&report)==0);
 assert(report.stop==PSP_STARTUP_VFPU_REQUIRED);
 assert(report.major_opcode==18u && report.executed==0u);
 reset(&cpu);
 w32(ram,0x44000000u); /* COP1/FPU major opcode */
 assert(psp_startup_trace_run(&cpu,&opts,&report)==0);
 assert(report.stop==PSP_STARTUP_FPU_REQUIRED);
 reset(&cpu);
 w32(ram,(9u<<6u)|12u); /* syscall #9, no fabricated result */
 assert(psp_startup_trace_run(&cpu,&opts,&report)==0);
 assert(report.stop==PSP_STARTUP_SYSCALL &&
        report.syscall_id==9u && report.executed==0u);
 assert(cpu.pc==BASE && cpu.fault==-3);
 reset(&cpu);
 w32(ram,0x8c080000u); /* LW t0,0(zero) outside guest memory */
 assert(psp_startup_trace_run(&cpu,&opts,&report)==0);
 assert(report.stop==PSP_STARTUP_MEMORY_FAULT);
 assert(report.interpreter_status==-2);
 reset(&cpu);
 cpu.pc=BASE+256u;cpu.next_pc=cpu.pc+4u;
 assert(psp_startup_trace_run(&cpu,&opts,&report)==0);
 assert(report.stop==PSP_STARTUP_OUTSIDE_CODE);
 assert(report.executed==0u);
 reset(&cpu);
 opts.instruction_cap=4u;
 assert(psp_startup_trace_run(&cpu,&opts,&report)==0);
 assert(report.stop==PSP_STARTUP_BUDGET &&
        report.executed==4u && cpu.pc==BASE+16u);
}
static void invalid_and_isolation(void){
 PspCpu cpu;reset(&cpu);
 PspStartupTraceConfig opts=config();
 PspStartupTraceReport report;
 assert(psp_startup_prepare_cpu(&cpu,8192u,4096u,0u)==-1);
 assert(psp_startup_prepare_cpu(&cpu,256u,2048u,0u)==-1);
 assert(psp_startup_prepare_cpu(&cpu,256u,PSP_STARTUP_MAX_STACK+16u,0u)==-1);
 assert(psp_startup_trace_run(NULL,&opts,&report)==-1);
 assert(psp_startup_trace_run(&cpu,NULL,&report)==-1);
 assert(psp_startup_trace_run(&cpu,&opts,NULL)==-1);
 opts.instruction_cap=0u;
 assert(psp_startup_trace_run(&cpu,&opts,&report)==-1);
 assert(report.executed==0u);
 opts=config();
 opts.executable[0].one_past_last=BASE+16384u;
 assert(psp_startup_trace_run(&cpu,&opts,&report)==-1);
 assert(report.executed==0u);
 PspStartupTraceConfig bad;
 assert(psp_startup_trace_config_elf(elf,sizeof elf,BASE,
                                    sizeof ram,0,&bad)==-1);
 assert(!bad.instruction_cap);
 assert(psp_startup_trace_config_elf(elf,sizeof elf,BASE,
                                    sizeof ram,PSP_STARTUP_TRACE_MAX_STEPS+1u,
                                    &bad)==-1);
 assert(psp_startup_trace_config_elf(elf,sizeof elf,BASE,
                                    sizeof ram,4u,NULL)==-1);
 assert(strcmp(psp_startup_stop_name(PSP_STARTUP_VFPU_REQUIRED),
               "vfpu_instruction_required")==0);
}
int main(void){
 fixture();
 known_import();
 first_missing_cpu_behavior();
 invalid_and_isolation();
 puts("PASS: deterministic bounded startup, stack, private-safe opcode classification and first firmware import");
 return 0;
}
