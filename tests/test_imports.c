#include "psp_imports.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#define BASE 0x08800000u
static unsigned rd32(const unsigned char *p) {
 return (unsigned)p[0]|(unsigned)p[1]<<8|(unsigned)p[2]<<16|(unsigned)p[3]<<24;
}
int main(void) {
 unsigned char memory[256]; memset(memory,0xaa,sizeof memory);
 PspCpu cpu; PspHle hle;
 psp_cpu_init(&cpu,memory,sizeof memory,BASE,BASE);psp_hle_init(&hle);
 const PspExport exports[]={
   {"UtilsForUser",0x11223344u,PSP_HLE_SYSTEM_TIME_LOW},
   {"UtilsForUser",0x55667788u,PSP_HLE_SYSTEM_TIME_WIDE},
   {"UtilsForUser",0x01020304u,PSP_HLE_SYSTEM_TIME}
 };
 const PspImport imports[]={
   {"UtilsForUser",0x11223344u,BASE+32},
   {"UtilsForUser",0x55667788u,BASE+48}
 };
 assert(psp_imports_link(&cpu,&hle,imports,2,exports,3)==0);
 assert(hle.count==2 && rd32(memory+32)==0x03e00008u);
 unsigned id0=rd32(memory+36)>>6,id1=rd32(memory+52)>>6;
 assert((rd32(memory+36)&63)==12 && id0!=id1);
 assert(hle.entries[0].syscall_id==id0 && hle.entries[1].syscall_id==id1);
 cpu.pc=BASE+32;cpu.next_pc=cpu.pc+4;cpu.gpr[31]=BASE+96;
 psp_hle_advance_time(&hle,0x100000005ULL);
 assert(psp_hle_step(&hle,&cpu)==0 && cpu.pc==BASE+36 && cpu.delay_slot);
 assert(psp_hle_step(&hle,&cpu)==1 && cpu.pc==BASE+96 && cpu.gpr[2]==5);
 cpu.pc=BASE+48;cpu.next_pc=cpu.pc+4;cpu.gpr[31]=BASE+100;
 assert(psp_hle_step(&hle,&cpu)==0);
 assert(psp_hle_step(&hle,&cpu)==1 && cpu.gpr[2]==5 && cpu.gpr[3]==1);
 PspImport bad[]={ {"UtilsForUser",0x01020304u,BASE+64},
                   {"UtilsForUser",0x99999999u,BASE+80} };
 size_t before=hle.count;
 assert(psp_imports_link(&cpu,&hle,bad,2,exports,3)==-3);
 assert(hle.count==before && rd32(memory+64)==0xaaaaaaaau);
 bad[1].nid=0x11223344u;bad[1].stub_address=BASE+68;
 assert(psp_imports_link(&cpu,&hle,bad,2,exports,3)==-2);
 assert(hle.count==before && rd32(memory+64)==0xaaaaaaaau);
 bad[1].stub_address=BASE+252;
 assert(psp_imports_link(&cpu,&hle,bad,2,exports,3)==-1);
 assert(hle.count==before && rd32(memory+64)==0xaaaaaaaau);
 PspExport ambiguous[]={exports[0],exports[0]};
 assert(psp_imports_link(&cpu,&hle,imports,1,ambiguous,2)==-2);
 assert(hle.count==before);
 hle.trace_unresolved_imports=1;
 bad[1].nid=0x99999999u;bad[1].stub_address=BASE+80;
 assert(psp_imports_link(&cpu,&hle,bad,2,exports,3)==0);
 assert(hle.count==before+1);
 assert(rd32(memory+84)==((PSP_HLE_UNRESOLVED_BASE+1u)<<6 | 12u));
 cpu.pc=BASE+80;cpu.next_pc=BASE+84;cpu.gpr[31]=BASE+100;
 assert(psp_hle_step(&hle,&cpu)==0);
 assert(psp_hle_step(&hle,&cpu)==-3);
 assert(cpu.last_trap_code==PSP_HLE_UNRESOLVED_BASE+1u);
 const PspImport sdk[]={{"SysMemUserForUser",0x358ca1bbu,BASE+112},
                        {"SysMemUserForUser",0xfc114573u,BASE+128},
                        {"SysMemUserForUser",0xf77d77cbu,BASE+144}};
 assert(psp_imports_link_known(&cpu,&hle,sdk,3)==0);
 psp_cpu_init(&cpu,memory,sizeof memory,BASE,BASE+112);
 cpu.gpr[4]=0x06060010u;cpu.gpr[31]=BASE+128;
 assert(psp_hle_step(&hle,&cpu)==0 && psp_hle_step(&hle,&cpu)==1);
 assert(cpu.gpr[2]==0 && cpu.pc==BASE+128);
 cpu.gpr[31]=BASE+144;
 assert(psp_hle_step(&hle,&cpu)==0 && psp_hle_step(&hle,&cpu)==1);
 assert(cpu.gpr[2]==0x06060010u);
 cpu.gpr[4]=0x040300u;cpu.gpr[31]=BASE+160;
 assert(psp_hle_step(&hle,&cpu)==0 && psp_hle_step(&hle,&cpu)==1);
 assert(hle.compiler_version==0x040300u && cpu.gpr[2]==0 && cpu.pc==BASE+160);
 const PspImport intr[]={{"Kernel_Library",0x092968f4u,BASE+160},
                         {"Kernel_Library",0x5f10d406u,BASE+168}};
 assert(psp_imports_link_known(&cpu,&hle,intr,2)==0);
 psp_cpu_init(&cpu,memory,sizeof memory,BASE,BASE+160);
 cpu.gpr[31]=BASE+160;
 assert(psp_hle_step(&hle,&cpu)==0 && psp_hle_step(&hle,&cpu)==1);
 assert(cpu.gpr[2]==1 && hle.interrupts_suspended);
 cpu.gpr[31]=BASE+168;
 assert(psp_hle_step(&hle,&cpu)==0 && psp_hle_step(&hle,&cpu)==1);
 assert(cpu.gpr[2]==0 && hle.interrupts_suspended);
 cpu.gpr[4]=2;cpu.gpr[31]=BASE+168;
 assert(psp_hle_step(&hle,&cpu)==0 && psp_hle_step(&hle,&cpu)==1);
 assert(hle.interrupts_suspended);
 cpu.gpr[4]=3;cpu.gpr[31]=BASE+176;
 assert(psp_hle_step(&hle,&cpu)==0 && psp_hle_step(&hle,&cpu)==1);
 assert(!hle.interrupts_suspended);
 unsigned char arena[2048]={0};
 psp_cpu_init(&cpu,arena,sizeof arena,BASE,BASE);
 hle.partition_low=BASE+257;hle.partition_high=BASE+1536;
 const PspImport memcalls[]={
  {"SysMemUserForUser",0x237dbd4fu,BASE},
  {"SysMemUserForUser",0x9d9a5ba1u,BASE+8},
  {"SysMemUserForUser",0xb6d61d02u,BASE+16}
 };
 assert(psp_imports_link_known(&cpu,&hle,memcalls,3)==0);
 cpu.gpr[4]=2;cpu.gpr[5]=BASE+32;cpu.gpr[6]=3;cpu.gpr[7]=257;
 cpu.gpr[8]=512;cpu.gpr[31]=BASE+8;
 assert(psp_hle_step(&hle,&cpu)==0 && psp_hle_step(&hle,&cpu)==1);
 unsigned uid=cpu.gpr[2];
 assert(uid>=0x2000 && uid<0x80000000u);
 cpu.gpr[4]=uid;cpu.gpr[31]=BASE+16;
 assert(psp_hle_step(&hle,&cpu)==0 && psp_hle_step(&hle,&cpu)==1);
 assert(cpu.gpr[2]==BASE+512);
 cpu.gpr[31]=BASE+8;
 assert(psp_hle_step(&hle,&cpu)==0 && psp_hle_step(&hle,&cpu)==1);
 assert(cpu.gpr[2]==0);
 cpu.gpr[31]=BASE+24;
 assert(psp_hle_step(&hle,&cpu)==0 && psp_hle_step(&hle,&cpu)==1);
 assert(cpu.gpr[2]==0);
 const PspImport stdio[]={{"StdioForUser",0x172d316eu,BASE+32},
                          {"StdioForUser",0xa6bab2e9u,BASE+40},
                          {"StdioForUser",0xf78ba90au,BASE+48}};
 assert(psp_imports_link_known(&cpu,&hle,stdio,3)==0);
 cpu.pc=BASE+32;cpu.next_pc=BASE+36;
 for(unsigned fd=0;fd<3;fd++){
  cpu.gpr[31]=BASE+40+fd*8;
  assert(psp_hle_step(&hle,&cpu)==0 && psp_hle_step(&hle,&cpu)==1);
  assert(cpu.gpr[2]==fd);
 }
 puts("PASS: import NID lookup, stub patching, rollback, jr-delay-slot HLE");
 return 0;
}
