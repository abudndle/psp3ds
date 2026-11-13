#include "psp_guest_bus.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#define H UINT32_C(0x08800000)
#define C UINT32_C(0x09000000)
int main(void){
 uint8_t hot[32],old_hot[32];uint16_t cold[32],old_cold[32];
 for(unsigned i=0;i<32;i++){hot[i]=(uint8_t)(i+1u);cold[i]=(uint16_t)(0x2000u+i);}
 memcpy(old_hot,hot,sizeof hot);memcpy(old_cold,cold,sizeof cold);
 PspGuestWindow ext;PspGuestBus bus;
 assert(psp_guest_window_init(&ext,cold,C,sizeof cold,1,1)==0);
 assert(psp_guest_bus_init(&bus,hot,H,sizeof hot,&ext)==0);
 uint8_t b;uint16_t h;uint32_t v;
 assert(psp_guest_bus_write32(&bus,H+4,0x78563412u)==0);
 assert(psp_guest_bus_read32(&bus,H+4,&v)==0&&v==0x78563412u);
 assert(psp_guest_bus_write8(&bus,H+5,0xaau)==0);
 assert(psp_guest_bus_read16(&bus,H+4,&h)==0&&h==0xaa12u);
 assert(psp_guest_bus_write32(&bus,C+4,0x12345678u)==0);
 assert(psp_guest_bus_read32(&bus,C+4,&v)==0&&v==0x12345678u);
 assert(psp_guest_bus_write8(&bus,C+5,0xaau)==0);
 assert(psp_guest_bus_read16(&bus,C+4,&h)==0&&h==0xaa78u);
 assert(psp_guest_bus_write16(&bus,C+sizeof cold-2,0xbeefu)==0);
 assert(psp_guest_bus_read16(&bus,C+sizeof cold-2,&h)==0&&h==0xbeefu);
 assert(psp_guest_bus_read8(&bus,H,&b)==0&&b==old_hot[0]);
 assert(psp_guest_bus_read8(&bus,C,&b)==0&&b==(uint8_t)old_cold[0]);
 /* A fault never touches either region, including boundary crossings. */
 uint8_t baseline_hot[32];uint16_t baseline_cold[32];
 memcpy(baseline_hot,hot,sizeof hot);memcpy(baseline_cold,cold,sizeof cold);
 assert(psp_guest_bus_write32(&bus,H+30,1)==-1);
 assert(psp_guest_bus_write16(&bus,C+sizeof cold-1,1)==-1);
 assert(psp_guest_bus_write32(&bus,C+sizeof cold,1)==-1);
 assert(psp_guest_bus_write8(&bus,H+sizeof hot,1)==-1);
 assert(psp_guest_bus_write8(&bus,C-1,1)==-1);
 assert(psp_guest_bus_read32(&bus,C+2,&v)==-1);
 assert(psp_guest_bus_read16(&bus,H+1,&h)==-1);
 assert(psp_guest_bus_read8(&bus,H,NULL)==-1);
 assert(memcmp(hot,baseline_hot,sizeof hot)==0);
 assert(memcmp(cold,baseline_cold,sizeof cold)==0);
 assert(psp_guest_bus_init(&bus,hot,H,sizeof hot,NULL)==0);
 assert(psp_guest_bus_read8(&bus,C,&b)==-1);
 assert(psp_guest_bus_write8(&bus,H,0x44)==0);
 assert(psp_guest_bus_init(&bus,hot,H,sizeof hot,&ext)==0);
 PspGuestWindow overlap=ext;overlap.guest_base=H+16;
 assert(psp_guest_bus_init(&bus,hot,H,sizeof hot,&overlap)==-1);
 assert(psp_guest_bus_write8(&bus,H,1)==-1);
 assert(psp_guest_bus_init(&bus,hot,UINT32_MAX-15u,sizeof hot,NULL)==-1);
 assert(psp_guest_bus_init(&bus,NULL,H,sizeof hot,NULL)==-1);
 puts("PASS: hot/cold guest data routing, endian, boundaries, no expansion fallback");
 return 0;
}
