#include <nds.h>
#include <stdio.h>
#include <stdint.h>
#include "psp_core.h"
/* DS/DSi ARM9-only shared core demo; not playable PSP emulation. */
int main(void) {
 consoleDemoInit();
 printf("PSP3DS / DS research port\n");
 printf("Shared CPU core demo\n\n");
 uint8_t ram[16]={0x01,0x00,0x08,0x24};
 PspCpu cpu;
 psp_cpu_init(&cpu,ram,sizeof ram,0x08800000u,0x08800000u);
 int ok=psp_cpu_step(&cpu)==0 && cpu.gpr[8]==1;
 printf("MIPS ADDIU: %s\n",ok?"OK":"FAIL");
 printf("Commercial PSP games: unsupported\n");
 printf("START to exit\n");
 while(1){swiWaitForVBlank();scanKeys();if(keysDown()&KEY_START)break;}
 return ok?0:1;
}
