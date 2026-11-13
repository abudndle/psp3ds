#include <nds.h>
#include <stdio.h>
#include <stdint.h>
#include "../../../include/psp_core.h"
/* Portable guest CPU demo; not a PSP commercial-game runner. */
static uint8_t test_ram[64];
static void test_shared_cpu(void){
 PspCpu cpu;
 uint32_t base=0x08800000u;
 const uint8_t instructions[]={1,0,8,0x24,1,0,8,0x25};
 for(unsigned i=0;i<sizeof(instructions);i++)test_ram[i]=instructions[i];
 psp_cpu_init(&cpu,test_ram,sizeof(test_ram),base,base);
 int a=psp_cpu_step(&cpu),b=psp_cpu_step(&cpu);
 printf("MIPS CPU demo: %s\n",a==0&&b==0&&cpu.gpr[8]==2?"PASS":"FAILED");
}
int main(void){
 consoleDemoInit();
 printf("PSP3DS / DS research\nMode: %s\n",isDSiMode()?"DSi-enhanced":"Nintendo DS");
 printf("Shared CPU; NOT PSP games\n\n");
 test_shared_cpu();
 printf("START: exit\n");
 while(1){scanKeys();if(keysDown()&KEY_START)break;swiWaitForVBlank();}
 return 0;
}
