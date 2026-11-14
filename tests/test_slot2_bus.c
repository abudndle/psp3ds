#include "psp_slot2_bus.h"
#include <assert.h>
#include <stdio.h>
int main(void){
 uint8_t hot[64]={0};
 uint16_t cold[4096]={0};
 PspGuestBus bus;PspPlatformBudget budget;
 PspSlot2Result probe={PSP_SLOT2_RW_VERIFIED,8192,16};
 const uint32_t H=0x08800000u,C=0x09000000u;
 assert(psp_slot2_bus_init(&bus,hot,H,sizeof hot,&probe,cold,C,&budget)==1);
 assert(budget.target==PSP_TARGET_DS_SLOT2);
 assert(budget.cold_data_budget==8192);
 assert(psp_guest_bus_write32(&bus,C+4,0x12345678u)==0);
 uint32_t value=0;
 assert(psp_guest_bus_read32(&bus,C+4,&value)==0 && value==0x12345678u);
 assert(psp_guest_bus_write8(&bus,H,0xabu)==0);
 assert(hot[0]==0xabu);
 assert(psp_slot2_bus_init(&bus,hot,H,sizeof hot,&probe,0,C,&budget)==0);
 assert(budget.target==PSP_TARGET_DS && !bus.has_cold);
 assert(psp_guest_bus_read32(&bus,C+4,&value)==-1);
 probe.status=PSP_SLOT2_UNVERIFIED;
 assert(psp_slot2_bus_init(&bus,hot,H,sizeof hot,&probe,cold,C,&budget)==0);
 assert(!bus.has_cold);
 probe.status=PSP_SLOT2_RW_VERIFIED;
 assert(psp_slot2_bus_init(&bus,hot,H,sizeof hot,&probe,cold,H+32,&budget)==-1);
 assert(psp_slot2_bus_init(0,hot,H,sizeof hot,&probe,cold,C,&budget)==-1);
 puts("PASS: verified Slot-2 guest data bus binding and safe fallback");
 return 0;
}
