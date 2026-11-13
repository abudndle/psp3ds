#include <nds.h>
#include <stdio.h>
#include <stdint.h>
#include <nds/arm9/peripherals/slot2.h>
#include "psp_slot2_probe.h"
#include "psp_benchmark.h"
#include "psp_guest_window.h"
#include "psp_guest_bus.h"
#include "psp_platform_budget.h"
#include "psp_features.h"
#include "psp_scene.h"
#include "psp_ge_subset.h"
int psp_nds_scene_init(void);
void psp_nds_scene_update(uint32_t frame);
#include "../../../include/psp_core.h"
/* Portable guest CPU demo; not a PSP commercial-game runner. */
static uint8_t test_ram[64];
static void test_shared_cpu(void){
 uint32_t value=0;
 int pass=psp_feature_run(PSP_FEATURE_CPU_SMOKE,&value)==0 && value==2u;
 printf("MIPS CPU demo: %s\n",pass?"PASS":"FAILED");
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
 if(result.status==PSP_SLOT2_RW_VERIFIED) {
  /* Sparse read/write sampling, NOT full RAM coverage or guest allocation. */
  PspSlot2Result sweep=psp_slot2_probe_samples(region,capacity,1,banks==1,32);
  printf("Sweep: %lu/32 %s\n",(unsigned long)sweep.probed_locations,
         sweep.status==PSP_SLOT2_RW_VERIFIED?"PASS":"FAILED");
  if(sweep.status==PSP_SLOT2_RW_VERIFIED){
   PspPlatformBudget expansion_policy;
   if(psp_platform_budget_choose(PSP_TARGET_DS_SLOT2,&expansion_policy)==0)
    printf("Pak cold cap: %luK\n",
           (unsigned long)(expansion_policy.cold_data_budget/1024u));
  }
  if(sweep.status==PSP_SLOT2_RW_VERIFIED && capacity>=256u){
   /* Test four 16-bit words only, restoring all original bytes before
    * closing the peripheral. No PSP CPU executes from this memory. */
   const uint32_t base=0x08800000u;
   PspGuestWindow guest;
   uint16_t saved[4];
   for(unsigned i=0;i<4;i++) saved[i]=region[i];
   int ok=psp_guest_window_init(&guest,region,base,256u,1,1)==0;
   uint32_t value=0;
   uint16_t halfword=0;
   if(ok) ok=psp_guest_window_write32(&guest,base,0x12345678u)==0;
   if(ok) ok=psp_guest_window_read32(&guest,base,&value)==0 && value==0x12345678u;
   if(ok) ok=psp_guest_window_write8(&guest,base+1u,0xabu)==0;
   if(ok) ok=psp_guest_window_read32(&guest,base,&value)==0 && value==0x1234ab78u;
   if(ok) ok=psp_guest_window_write16(&guest,base+6u,0xcdefu)==0;
   if(ok) ok=psp_guest_window_read16(&guest,base+6u,&halfword)==0 && halfword==0xcdefu;
   if(ok) ok=psp_guest_window_read32(&guest,base+254u,&value)!=0;
   for(unsigned i=0;i<4;i++) region[i]=saved[i];
   for(unsigned i=0;i<4;i++) if(region[i]!=saved[i])ok=0;
   printf("Guest data I/O: %s\n",ok?"PASS":"FAILED");
   if(ok) {
    uint8_t hot[16]={0};
    PspGuestWindow cold_window;
    PspGuestBus bus;
    int routed=psp_guest_window_init(&cold_window,region,
                   0x09000000u,256u,1,1)==0;
    if(routed) routed=psp_guest_bus_init(&bus,hot,0x08800000u,
                                   sizeof hot,&cold_window)==0;
    uint32_t cold_value=0,hot_value=0;
    if(routed) routed=psp_guest_bus_write32(&bus,0x08800000u,0x10203040u)==0;
    if(routed) routed=psp_guest_bus_write32(&bus,0x09000000u,0x50607080u)==0;
    if(routed) routed=psp_guest_bus_read32(&bus,0x08800000u,&hot_value)==0 &&
                               hot_value==0x10203040u;
    if(routed) routed=psp_guest_bus_read32(&bus,0x09000000u,&cold_value)==0 &&
                               cold_value==0x50607080u;
    if(routed) routed=psp_guest_bus_write8(&bus,0x09000100u,0)==-1;
    for(unsigned i=0;i<4;i++)region[i]=saved[i];
    for(unsigned i=0;i<4;i++)if(region[i]!=saved[i])routed=0;
    printf("Dual RAM bus: %s\n",routed?"PASS":"FAILED");
   }
  }
 }
 /* Never present RAM as a general C heap. Close it after the probe. */
 peripheralSlot2Close();
 peripheralSlot2Exit();
}
/* TIMER0 counts at approximately 32768Hz (ARM9 ~33.51 MHz/1024).
 * This is a local device-timer measurement only, never commercial PSP FPS. */
static void benchmark_shared_mips(void){
 TIMER0_CR=0;
 TIMER0_DATA=0;
 TIMER0_CR=TIMER_ENABLE|TIMER_DIV_1024;
 uint32_t executed=0;
 int status=psp_feature_run(PSP_FEATURE_MIPS_BENCHMARK,&executed);
 uint16_t ticks=TIMER0_DATA;
 TIMER0_CR=0;
 printf("MIPS %lu insns: %s\n",(unsigned long)executed,
        status==0?"PASS":"FAILED");
 printf("TIMER0 /1024: %u ticks\n",(unsigned)ticks);
 printf("Synthetic CPU test only\n");
}
int main(void){
 consoleDemoInit();
 int scene_ok=psp_nds_scene_init()==0;
 printf("PSP3DS / DS research\nMode: %s\n",isDSiMode()?"DSi-enhanced":"Nintendo DS");
 printf("Shared CPU; NOT PSP games\n\n");
 PspPlatformBudget policy;
 PspTarget target=isDSiMode()?PSP_TARGET_DSI:PSP_TARGET_DS;
 if(psp_platform_budget_choose(target,&policy)==0)
  printf("Policy: %s; hot %luK\n",policy.name,
         (unsigned long)(policy.hot_ram_budget/1024u));
 PspFeatureManifest features;
 if(psp_features_manifest(target,&features)==0){
  printf("Features: %u same IDs\n",(unsigned)PSP_FEATURE_COUNT);
  printf("PSP game: %s\n",
         psp_feature_state_name(features.features[PSP_FEATURE_PSP_GAME_BOOT].state));
 }


 /* Same experimental GE command decoder as original/New 3DS. */
 {
  const uint32_t words[]={UINT32_C(0x04060002),UINT32_C(0x0f000000),
                          UINT32_C(0x0c000000)};
  const PspGeDecodedSprite sprite={32,70,8,8,0x7c1f};
  PspGeSubsetResult ge;
  int accepted=psp_ge_subset_translate(words,3,&sprite,1,&ge)==0 &&
               ge.batch.count==1 &&
               ge.batch.rects[0].x==32;
  printf("GE sprite subset: %s\n",accepted?"PASS":"FAILED");
 }
 test_shared_cpu();
 benchmark_shared_mips();
 test_slot2_ram();
 printf("Scene BG/OAM: %s\n",scene_ok?"PASS":"FAILED");
 printf("START: exit\n");
 uint32_t frame=0;
 while(1){
  scanKeys();if(keysDown()&KEY_START)break;
  swiWaitForVBlank();
  if(scene_ok)psp_nds_scene_update(frame++);
 }
 return 0;
}
