#include "psp_imports.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#define BASE 0x08800000u
static uint32_t word(const uint8_t *p){
 return (uint32_t)p[0] | (uint32_t)p[1]<<8 |
        (uint32_t)p[2]<<16 | (uint32_t)p[3]<<24;
}
int main(void){
 uint8_t ram[512];memset(ram,0xaa,sizeof ram);
 PspCpu cpu;PspHle hle;
 psp_cpu_init(&cpu,ram,sizeof ram,BASE,BASE);psp_hle_init(&hle);
 const PspImport imports[]={
  {"IoFileMgrForUser",0x109f50bcu,BASE+32},
  {"IoFileMgrForUser",0x6a638d83u,BASE+48},
  {"IoFileMgrForUser",0x68963324u,BASE+64},
  {"IoFileMgrForUser",0x810c4bc3u,BASE+80}
 };
 assert(psp_imports_link_known(&cpu,&hle,imports,4)==0 && hle.count==4);
 const PspHleService services[]={
  PSP_HLE_IO_OPEN,PSP_HLE_IO_READ,PSP_HLE_IO_LSEEK32,PSP_HLE_IO_CLOSE
 };
 for(unsigned i=0;i<4;i++){
  unsigned off=32+i*16;
  assert(word(ram+off)==0x03e00008u);
  assert((word(ram+off+4)&63)==12);
  assert(hle.entries[i].syscall_id==(word(ram+off+4)>>6));
  assert(hle.entries[i].service==services[i]);
 }
 /* Guest executes jr $ra followed by an imported syscall. Without a
  * mounted IO context, the call safely returns an error, not an unhandled
  * MIPS exception. */
 cpu.pc=BASE+32;cpu.next_pc=BASE+36;cpu.gpr[31]=BASE+160;
 cpu.gpr[4]=BASE+200;memcpy(ram+200,"disc0:/TEST",12);cpu.gpr[5]=1;
 assert(psp_hle_step(&hle,&cpu)==0 && cpu.delay_slot);
 assert(psp_hle_step(&hle,&cpu)==1 && cpu.pc==BASE+160 &&
        cpu.gpr[2]==UINT32_MAX);
 const PspImport bad[]={
  {"IoFileMgrForUser",0x109f50bcu,BASE+112},
  {"IoFileMgrForUser",0xdeadbeefu,BASE+128}
 };
 assert(psp_imports_link_known(&cpu,&hle,bad,2)==-3);
 assert(hle.count==4 && word(ram+112)==0xaaaaaaaau);
 PspImport wrong={"IoFileMgrForKernel",0x109f50bcu,BASE+112};
 assert(psp_imports_link_known(&cpu,&hle,&wrong,1)==-3);
 assert(word(ram+112)==0xaaaaaaaau);
 puts("PASS: verified IoFileMgr NIDs, MIPS import stubs, fail-closed linking");
 return 0;
}
