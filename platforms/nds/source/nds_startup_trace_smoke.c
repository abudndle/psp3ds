/* Portable Allegrex startup checkpoint on the actual DS/DSi ARM9.
 * Original synthetic code only, no PSP games, no large guest allocation.
 * A verified Memory Pak changes cold storage only, not this CPU path. */
#include "psp_startup_trace.h"
#include <stdint.h>
#include <string.h>

#define DS_TRACE_BASE UINT32_C(0x08800000)
static uint8_t code[4096];
static uint8_t elf[128];
static void word(uint8_t *p,uint32_t value){
 for(unsigned i=0;i<4;i++)p[i]=(uint8_t)(value>>(i*8u));
}
static void half(uint8_t *p,uint16_t value){
 p[0]=(uint8_t)value;p[1]=(uint8_t)(value>>8u);
}
int psp_nds_startup_trace_smoke(void){
 memset(code,0,sizeof code);
 memset(elf,0,sizeof elf);
 memcpy(elf,"\177ELF",4u);elf[4]=1;elf[5]=1;elf[6]=1;
 half(elf+16,0xffa0u);half(elf+18,8u);
 word(elf+20,1u);word(elf+24,0u);word(elf+28,52u);
 half(elf+40,52u);half(elf+42,32u);half(elf+44,1u);
 word(elf+52,1u); /* PT_LOAD */
 word(elf+56,84u); /* file */
 word(elf+60,0u); /* relative guest offset */
 word(elf+68,16u);word(elf+72,16u);
 word(elf+76,5u); /* file-backed and executable */
 word(code,0x27bdfff0u); /* addiu sp,sp,-16 */
 word(code+4,0x25080001u); /* addiu t0,t0,1 */
 word(code+8,(5u<<6u)|12u); /* unbound PSP syscall #5 */
 PspCpu cpu;
 PspStartupTraceConfig plan;
 PspStartupTraceReport trace;
 psp_cpu_init(&cpu,code,sizeof code,DS_TRACE_BASE,DS_TRACE_BASE);
 if(psp_startup_trace_config_elf(elf,sizeof elf,DS_TRACE_BASE,
                       sizeof code,16u,&plan)!=0 ||
    psp_startup_prepare_cpu(&cpu,16u,PSP_STARTUP_MIN_STACK,0u)!=0 ||
    psp_startup_trace_run(&cpu,&plan,&trace)!=0)return -1;
 if(trace.stop!=PSP_STARTUP_SYSCALL ||trace.executed!=2u ||
    trace.syscall_id!=5u ||trace.stop_pc!=DS_TRACE_BASE+8u ||
    cpu.gpr[8]!=1u ||cpu.gpr[29]!=DS_TRACE_BASE+sizeof code-32u)
  return -1;
 /* Repeated run from same initial state must have the same path hash. */
 const uint32_t fingerprint=trace.pc_fingerprint;
 memset(code,0,sizeof code);
 word(code,0x27bdfff0u);word(code+4,0x25080001u);
 word(code+8,(5u<<6u)|12u);
 psp_cpu_init(&cpu,code,sizeof code,DS_TRACE_BASE,DS_TRACE_BASE);
 if(psp_startup_prepare_cpu(&cpu,16u,PSP_STARTUP_MIN_STACK,0u)!=0 ||
    psp_startup_trace_run(&cpu,&plan,&trace)!=0 ||
    trace.pc_fingerprint!=fingerprint)return -1;
 return 0;
}
