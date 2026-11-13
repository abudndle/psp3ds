#include <nds.h>
#include <stdio.h>
#include <stdint.h>
#include <nds/arm9/peripherals/slot2.h>
#include "psp_slot2_probe.h"
#include "psp_benchmark.h"
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
static void test_slot2_ram(void){
 if(isDSiMode()){
  printf("Slot-2: unavailable (DSi)\n");
  return;
 }
 if(!peripheralSlot2Init(SLOT2_PERIPHERAL_EXTRAM)){
  printf("Slot-2: no RAM detected\n");
  return;
 }
 if(!peripheralSlot2Open(SLOT2_PERIPHERAL_EXTRAM)){
  printf("Slot-2: cannot unlock RAM\n");
  peripheralSlot2Exit();
  return;
 }
 uint32_t capacity=peripheralSlot2RamSize();
 uint32_t banks=peripheralSlot2RamBanks();
 volatile uint16_t *region=(volatile uint16_t*)peripheralSlot2RamStart();
 PspSlot2Result result=psp_slot2_probe(region,capacity,1,banks==1);
 printf("Slot-2: %s\n",peripheralSlot2GetName());
 printf("RAM: %lu KiB, banks: %lu\n",
        (unsigned long)(capacity/1024u),(unsigned long)banks);
 printf("Read/write: %s\n",result.status==PSP_SLOT2_RW_VERIFIED?
        "2 locations verified":"NOT VERIFIED");
 /* Never present RAM as a general C heap. Close it after the probe. */
 peripheralSlot2Close();
 peripheralSlot2Exit();
}
/* TIMER0 counts at approximately 32768Hz (ARM9 ~33.51 MHz/1024).
 * This is a local device-timer measurement only, never commercial PSP FPS. */
static void benchmark_shared_mips(void){
 PspBenchResult result;
 TIMER0_CR=0;
 TIMER0_DATA=0;
 TIMER0_CR=TIMER_ENABLE|TIMER_DIV_1024;
 int status=psp_benchmark_run(16,&result);
 uint16_t ticks=TIMER0_DATA;
 TIMER0_CR=0;
 printf("MIPS %lu insns: %s\n",(unsigned long)result.executed_instructions,
        status==0?"PASS":"FAILED");
 printf("TIMER0 /1024: %u ticks\n",(unsigned)ticks);
 printf("Synthetic CPU test only\n");
}
int main(void){
 consoleDemoInit();
 printf("PSP3DS / DS research\nMode: %s\n",isDSiMode()?"DSi-enhanced":"Nintendo DS");
 printf("Shared CPU; NOT PSP games\n\n");
 test_shared_cpu();
 benchmark_shared_mips();
 test_slot2_ram();
 printf("START: exit\n");
 while(1){scanKeys();if(keysDown()&KEY_START)break;swiWaitForVBlank();}
 return 0;
}
