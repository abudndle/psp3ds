#include <nds.h>
#include <stdio.h>
#include <stdint.h>
#include <nds/arm9/peripherals/slot2.h>
#include "psp_slot2_probe.h"
#include "psp_benchmark.h"
#include "psp_guest_window.h"
#include "psp_guest_bus.h"
#include "psp_rom_pager.h"
#include "psp_platform_budget.h"
#include "psp_ds_memory.h"
#include "psp_runtime_tuning.h"
#include "psp_video_layout.h"
#include "psp_native_texture.h"
#include "psp_nintendo_input.h"
#include "psp_features.h"
#include "psp_scene.h"
#include "psp_ge_subset.h"
#include "psp_ge_triangles.h"
int psp_nds_scene_init(void);
void psp_nds_scene_update(uint32_t frame);
int psp_nds_triangles_init(void);
void psp_nds_triangles_update(void);
void psp_nds_triangles_shutdown(void);
int psp_nds_rom_pager_synthetic_test(void);
void psp_nds_memory_report(PspTarget target,
                           uint32_t sampled_slot2_bytes,
                           int sampled_slot2_verified);
int psp_nds_rom_pager_verified_slot2_test(volatile uint16_t *region,
                                           uint32_t capacity);
#include "../../../include/psp_core.h"
/* Portable guest CPU demo; not a PSP commercial-game runner. */
static uint8_t test_ram[64];
/* Metadata only. The Slot-2 peripheral is CLOSED after test_slot2_ram(). */
static uint32_t verified_slot2_capacity_bytes;
static void test_shared_cpu(void){
 uint32_t value=0;
 int pass=psp_feature_run(PSP_FEATURE_CPU_SMOKE,&value)==0 && value==2u;
 printf("MIPS CPU demo: %s\n",pass?"PASS":"FAILED");
}
static int test_slot2_ram(void){
 int verified_pak=0;
 verified_slot2_capacity_bytes=0;
 if(isDSiMode()){
  printf("Slot-2: unavailable (DSi)\n");
  return 0;
 }
 if(!peripheralSlot2Init(SLOT2_PERIPHERAL_EXTRAM)){
  printf("Slot-2: no RAM detected\n");
  return 0;
 }
 if(!peripheralSlot2Open(SLOT2_PERIPHERAL_EXTRAM)){
  printf("Slot-2: cannot unlock RAM\n");
  peripheralSlot2Exit();
  return 0;
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
    if(routed){
     const int paged=psp_nds_rom_pager_verified_slot2_test(region,capacity);
     printf("Pak cold ROM pages: %s\n",
            paged==0?"PASS (synthetic)":"FAILED");
     /* Only select the Pak profile when all previous checks, including
      * a fully restored 8-KiB test window, succeeded. This does not
      * prove the entire claimed peripheral memory region is writable. */
     if(paged==0){
      verified_pak=1;
      verified_slot2_capacity_bytes=capacity;
     }
    }
   }
  }
 }
 /* Never present RAM as a general C heap. Close it after the probe. */
 peripheralSlot2Close();
 peripheralSlot2Exit();
 return verified_pak;
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
_Static_assert(KEY_A == PSP_NINTENDO_KEY_A &&
               KEY_B == PSP_NINTENDO_KEY_B &&
               KEY_UP == PSP_NINTENDO_KEY_UP &&
               KEY_RIGHT == PSP_NINTENDO_KEY_RIGHT &&
               KEY_X == PSP_NINTENDO_KEY_X,
               "BlocksDS Nintendo key positions have changed");
int main(void){
 consoleDemoInit();
 int scene_ok=psp_nds_scene_init()==0;
 printf("PSP3DS / DS research\nMode: %s\n",isDSiMode()?"DSi-enhanced":"Nintendo DS");
 printf("Shared CPU; NOT PSP games\n\n");
 /* Select DS+Pak profile only after 32-location read/write validation
  * and a passing hot/cold guest data-bus probe. */
 const int slot2_verified=test_slot2_ram();
 printf("512MB ROM page index: %s\n",
        psp_nds_rom_pager_synthetic_test()==0?
        "PASS (synthetic)":"FAILED");
 PspPlatformBudget policy;
 PspTarget target=isDSiMode()?PSP_TARGET_DSI:
                  slot2_verified?PSP_TARGET_DS_SLOT2:PSP_TARGET_DS;
 psp_nds_memory_report(target,verified_slot2_capacity_bytes,slot2_verified);
 PspRuntimeTuning tuning;
 if(psp_runtime_tuning_choose(target,&tuning)==0) {
  printf("Guest slice: %lu, GPU batch cap %u\n",
         (unsigned long)tuning.cpu_slice_current,
         (unsigned)tuning.triangle_demo_budget);
  printf("Clock boost: %s\n",
         tuning.request_new3ds_speedup?"unsupported":"disabled");
 }

 PspVideoLayout video;
 if(psp_video_layout_init(target,PSP_VIDEO_DISPLAY_DS_MAIN,
                          PSP_VIDEO_PSP_WIDTH,PSP_VIDEO_PSP_HEIGHT,&video)==0)
  printf("PSP video: %ux%u; DS tiles\n",
    (unsigned)video.viewport_width,(unsigned)video.viewport_height);
 if(psp_platform_budget_choose(target,&policy)==0)
  printf("Policy: %s; hot %luK\n",policy.name,
         (unsigned long)(policy.hot_ram_budget/1024u));
 /* Exercise portable GE texture conversion with a tiny original
  * fixture; real PSP game textures are not rendered by this diagnostic. */
 {
  const PspNativeTexture tex={2,1,2,PSP_TEXTURE_5650,0};
  const PspNativeTextureRegion rect={0,0,2,1};
  const uint8_t bytes[]={0x1f,0,0xe0,0x07};
  uint8_t rgba[8]={0};
  const int decoded=psp_native_texture_decode_region(
    &tex,bytes,sizeof bytes,&rect,rgba,8,sizeof rgba)==PSP_TEXTURE_OK &&
    rgba[0]==255 && rgba[1]==0 && rgba[2]==0 && rgba[3]==255 &&
    rgba[4]==0 && rgba[5]==255 && rgba[6]==0 && rgba[7]==255;
  printf("PSP texture decode: %s\n",decoded?"PASS":"FAILED");
 }
 /* Synthetic PSP through-mode triangle command diagnostic, shared by
  * DS, DS+verified Slot-2, DSi and old/New 3DS. Not game execution. */
 {
  const uint32_t cmds[]={0x10080000u,0x01000000u,0x12800114u,
                         0x04030003u,0x0f000000u,0x0c000000u};
  const uint8_t verts[]={
   0x1f,0x80,20,0,20,0,1,0,
   0xe0,0x83,180,0,30,0,2,0,
   0x00,0xfc,100,0,150,0,3,0
  };
  PspGeTriangleResult tris;
  const int ok=psp_ge_triangles_decode(cmds,6,verts,sizeof verts,
                                      0x08000000u,&tris)==PSP_GE_TRIANGLE_OK &&
               tris.count==1u && tris.triangles[0].v[2].z==3u;
  printf("GE triangles: %s\n",ok?"PASS":"FAILED");
 }
 PspFeatureManifest features;
 if(psp_features_manifest(target,&features)==0){
  printf("Features: %u same IDs\n",(unsigned)PSP_FEATURE_COUNT);
  printf("PSP game: %s\n",
         psp_feature_state_name(features.features[PSP_FEATURE_PSP_GAME_BOOT].state));
 }


 /* Same experimental GE command decoder as original/New 3DS. */
 {
  const uint32_t words[] = {0x10080000u, 0x01000000u, 0x12800114u,
                            0x04060002u, 0x0f000000u, 0x0c000000u};
  const uint8_t vertices[] = {0x1f, 0xfc, 32, 0, 70, 0, 0, 0,
                              0x1f, 0xfc, 40, 0, 78, 0, 0, 0};
  PspGeSubsetResult ge;
  int accepted = psp_ge_subset_translate_memory(words, 6, vertices,
                  sizeof vertices, 0x08000000u, &ge) == PSP_GE_SUBSET_OK &&
               ge.batch.count==1 &&
               ge.batch.rects[0].x==32;
  printf("GE sprite subset: %s\n",accepted?"PASS":"FAILED");
 }
 test_shared_cpu();
 benchmark_shared_mips();
 printf("Scene BG/OAM: %s\n",scene_ok?"PASS":"FAILED");
 printf("Y: DS native 3D triangles\n");
 printf("START: exit\n");
 uint32_t frame=0;
 int triangle_mode=0;
 while(1){
  scanKeys();
  const uint32_t pressed=keysDown();
  PspNintendoCtrl psp_pad;
  if(psp_nintendo_ctrl_sample(target,keysHeld(),0,0,&psp_pad)==0 &&
     (pressed&KEY_SELECT)){
   printf("PSP pad: %04lx X:%u Y:%u\n",
          (unsigned long)psp_pad.buttons,(unsigned)psp_pad.lx,
          (unsigned)psp_pad.ly);
  }
  if(pressed&KEY_START)break;
  if(pressed&KEY_Y){
   if(!triangle_mode)triangle_mode=psp_nds_triangles_init()==0;
   else {psp_nds_triangles_shutdown();triangle_mode=0;}
  }
  swiWaitForVBlank();
  if(triangle_mode)psp_nds_triangles_update();
  else if(scene_ok)psp_nds_scene_update(frame++);
 }
 psp_nds_triangles_shutdown();
 return 0;
}
