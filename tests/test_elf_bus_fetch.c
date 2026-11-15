/* End-to-end synthetic ELF -> checked guest-bus fetch integration.
 * The CPU interpreter is NOT PPSSPP; this tests the portable memory boundary. */
#include "psp_loader.h"
#include "psp_ppsspp_bus_bridge.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define BASE UINT32_C(0x08800000)
static void w16(uint8_t *p,uint16_t v){p[0]=(uint8_t)v;p[1]=(uint8_t)(v>>8);}
static void w32(uint8_t *p,uint32_t v){
 for(unsigned i=0;i<4;i++)p[i]=(uint8_t)(v>>(8*i));
}
int main(void){
 uint8_t elf[256],ram[4096];
 PspLoadResult loaded;
 PspGuestBus bus;
 uint32_t instruction=0;
 memset(elf,0,sizeof elf);
 memcpy(elf,"\177ELF",4);elf[4]=1;elf[5]=1;elf[6]=1;
 w16(elf+16,2);w16(elf+18,8);w32(elf+20,1);
 w32(elf+24,BASE);w32(elf+28,52);
 w16(elf+40,52);w16(elf+42,32);w16(elf+44,1);
 w32(elf+52,1);w32(elf+56,128);w32(elf+60,BASE);
 w32(elf+68,8);w32(elf+72,8);w32(elf+76,5);w32(elf+80,4);
 w32(elf+128,UINT32_C(0x24080007)); /* addiu t0,zero,7 */
 w32(elf+132,UINT32_C(0x0000000c)); /* syscall */
 memset(ram,0xcc,sizeof ram);
 assert(psp_elf_load(elf,sizeof elf,ram,sizeof ram,BASE,&loaded)==PSP_LOAD_OK);
 assert(loaded.entry==BASE && loaded.segments==1);
 assert(psp_guest_bus_init(&bus,ram,BASE,sizeof ram,NULL)==0);
 assert(psp_ppsspp_bus_fetch32(&bus,loaded.entry,&instruction)==0);
 assert(instruction==UINT32_C(0x24080007));
 assert(psp_ppsspp_bus_fetch32(&bus,loaded.entry+4,&instruction)==0);
 assert(instruction==UINT32_C(0x0000000c));
 assert(psp_ppsspp_bus_fetch32(&bus,loaded.entry+1,&instruction)==-1);
 assert(psp_ppsspp_bus_fetch32(&bus,BASE+sizeof ram,&instruction)==-1);
 puts("PASS: synthetic ELF loader -> checked guest bus instruction fetch (NOT PPSSPP execution)");
 return 0;
}
