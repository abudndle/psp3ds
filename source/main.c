#include <3ds.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "psp_core.h"
#include "psp_catalog.h"
#include "psp_platform_budget.h"
#include "psp_3ds_memory.h"
#include "psp_3ds_storage.h"
#include "psp_runtime_tuning.h"
#include "psp_benchmark.h"
#include "psp_video_layout.h"
#include "psp_native_texture.h"
#include "psp_ge_triangles.h"
#include "psp_nintendo_input.h"
#include "psp_features.h"
#include "psp_scene.h"
int psp_3ds_scene_init(void);
int psp_3ds_scene_draw(uint32_t frame);
int psp_3ds_triangle_draw(uint32_t frame,PspTarget model);
int psp_3ds_guest_runtime_demo(PspTarget model);
int psp_3ds_native_memory_probe(PspTarget model);
int psp_3ds_native_sd_probe(void);
void psp_3ds_scene_fini(void);
static const char *type_name(PspFileType t) {
 switch(t) {case PSP_FILE_PBP:return "PBP";case PSP_FILE_ELF:return "MIPS ELF";case PSP_FILE_ISO:return "ISO";case PSP_FILE_CSO:return "CSO";default:return "Unknown";}
}
/* Boot-test signature: three highly saturated bars on the lower LCD.
 * This only proves that our own 3DS homebrew frame is displayed by Azahar.
 * It is not PSP game execution or a general 3DS hardware compatibility test.
 * libctru's default GSP_BGR8_OES framebuffer has 240x320 rotated pixels. */
static void psp3ds_draw_boot_marker(void) {
 u16 width=0,height=0;
 u8 *fb=gfxGetFramebuffer(GFX_BOTTOM,GFX_LEFT,&width,&height);
 if(!fb || !width || !height)return;
 for(unsigned y=0;y<height;y++){
  unsigned band=(3u*y)/height;
  unsigned char b=band==0?255:band==1?255:0;
  unsigned char g=band==0?0:255;
  unsigned char r=band==1?0:255;
  for(unsigned x=0;x<width;x++){
   size_t offset=((size_t)y*width+x)*3u;
   fb[offset]=b;
   fb[offset+1]=g;
   fb[offset+2]=r;
  }
 }
}
_Static_assert(KEY_A == PSP_NINTENDO_KEY_A &&
               KEY_B == PSP_NINTENDO_KEY_B &&
               KEY_DUP == PSP_NINTENDO_KEY_UP &&
               KEY_DRIGHT == PSP_NINTENDO_KEY_RIGHT &&
               KEY_X == PSP_NINTENDO_KEY_X,
               "libctru Nintendo key positions have changed");
int main(void) {
 /* Only request the New 3DS clock increase once APT confirms hardware. */
 gfxInitDefault();consoleInit(GFX_TOP,NULL);
 printf("PSP3DS | research prototype\n");
 printf("PSP file inspection + basic MIPS interpreter\n");
 printf("Not a playable PSP emulator yet.\n\n");
 bool new_model=false;
 Result hardware=APT_CheckNew3DS(&new_model);
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
 PspPlatformBudget policy;
 PspTarget target=(R_SUCCEEDED(hardware) && new_model)?
    PSP_TARGET_NEW3DS:PSP_TARGET_OLD3DS;
 PspRuntimeTuning tuning;
 const int tune_ok=psp_runtime_tuning_choose(target,&tuning)==0;
 const bool high_clock=tune_ok && tuning.request_new3ds_speedup;
 if(high_clock)osSetSpeedupEnable(true);
 /* A CPU speedup request does not grant the app the physical 256 MiB.
  * libctru reports the process APPLICATION memory region and free bytes.
  * The scratch allocation is always released before other runtime work. */
 if(R_SUCCEEDED(hardware)){
  const int memory_probe=psp_3ds_native_memory_probe(target);
  printf("3DS RAM probe: %s\n",memory_probe==0?"PASS":"limited");
 }else{
  printf("3DS model unavailable: memory probe skipped\n");
 }
 /* Optional SD file under /psp3ds; FSFILE_Read uses 64-bit offsets.
  * Never prints or executes the contents. No file is required to boot. */
 (void)psp_3ds_native_sd_probe();
 /* Execute an original synthetic MIPS syscall and an HLE-aware guest
  * instruction slice using the actual per-model scheduler. */
 const int guest_demo_ok=psp_3ds_guest_runtime_demo(target)==0;
 printf("Native guest scheduler: %s\n",guest_demo_ok?"PASS":"FAILED");

 if(tune_ok){
  PspBenchResult bench;
  TickCounter ticks={0};
  osTickCounterStart(&ticks);
  const int bench_status=psp_benchmark_run(tuning.benchmark_rounds,&bench);
  osTickCounterUpdate(&ticks);
  const double elapsed_ms=osTickCounterRead(&ticks);
  printf("CPU mode: %s\n",high_clock?"New 3DS speedup requested":"standard");
  printf("Geometry batch: %u, decode staging %luK\n",
         (unsigned)tuning.triangle_demo_budget,
         (unsigned long)(tuning.texture_decode_staging_bytes/1024u));
  if(bench_status==0 && elapsed_ms>0.0 && elapsed_ms<60000.0){
   uint32_t elapsed_us=(uint32_t)(elapsed_ms*1000.0+0.5);
   if(!elapsed_us)elapsed_us=1;
   const int adapted=psp_runtime_tuning_feedback(&tuning,
                       bench.executed_instructions,elapsed_us);
   printf("Synthetic %lu MIPS insns: %.2f ms\n",
          (unsigned long)bench.executed_instructions,elapsed_ms);
   printf("CPU next-slice cap: %lu (%s)\n",
          (unsigned long)tuning.cpu_slice_current,
          adapted>=0?"adaptive policy":"unchanged");
  }else{
   printf("Synthetic MIPS timing: unavailable\n");
  }
 }
 PspVideoLayout video;
 if(psp_video_layout_init(target,PSP_VIDEO_DISPLAY_3DS_TOP,
                          PSP_VIDEO_PSP_WIDTH,PSP_VIDEO_PSP_HEIGHT,&video)==0)
  printf("PSP video: %ux%u viewport; PICA200\n",
    (unsigned)video.viewport_width,(unsigned)video.viewport_height);
 if(psp_platform_budget_choose(target,&policy)==0 &&
    psp_platform_budget_validate(&policy)==0){
  printf("Policy: %s\n",policy.name);
  printf("Hot cap %luK, texture cap %luK\n",
    (unsigned long)(policy.hot_ram_budget/1024u),
    (unsigned long)(policy.texture_budget/1024u));
  printf("Sprite batching planned\n");
  PspFeatureManifest features;
  if(psp_features_manifest(target,&features)==0){
   uint32_t cpu_value=0,bench_value=0;
   int cpu_ok=psp_feature_run(PSP_FEATURE_CPU_SMOKE,&cpu_value)==0 &&
              cpu_value==2u;
   int bench_ok=psp_feature_run(PSP_FEATURE_MIPS_BENCHMARK,&bench_value)==0 &&
                bench_value==2048u;
   printf("Parity CPU: %s\n",cpu_ok?"PASS":"FAILED");
   printf("MIPS %lu insns: %s\n",(unsigned long)bench_value,
          bench_ok?"PASS":"FAILED");
   printf("Features: %u same IDs\n",(unsigned)PSP_FEATURE_COUNT);
   printf("PSP game: %s\n",
          psp_feature_state_name(features.features[PSP_FEATURE_PSP_GAME_BOOT].state));
  }

 }

 /* Scan ROM names without loading or executing their contents. */
 static PspCatalogItem games[12];
 size_t count=0;
 int found=psp_catalog_scan("sdmc:/psp/roms",games,12,&count);
 if(found==-1)printf("No SD /psp/roms directory found.\\n");
 else {
  printf("ROM images: %lu%s\\n",(unsigned long)count,found==-2?"+":"");
  for(size_t i=0;i<count&&i<6;i++)printf("%s\\n",games[i].filename);
  printf("ROM execution is not yet supported.\\n");
 }
 printf("Checking legacy sdmc:/psp3ds/EBOOT.PBP\\n");
 FILE *f=fopen("sdmc:/psp3ds/EBOOT.PBP","rb");
 if(f) {
  if(fseek(f,0,SEEK_END)==0) {
   long n=ftell(f);
   if(n>=40 && fseek(f,0,SEEK_SET)==0) {
    /* Read metadata only. Previously this diagnostic allocated up to
     * 8 MiB for the entire EBOOT.PBP and could consume precious app RAM.
     * Only PBP/ELF headers are inspected here; no PSP game is loaded. */
    unsigned char header[52]={0};
    const size_t got=fread(header,1,sizeof header,f);
    if(got>=40u &&
       ((memcmp(header,"\0PBP",4)==0) ||
        (got>=52u && memcmp(header,"\177ELF",4)==0))){
     PspFileInfo info;
     const int rc=psp_inspect(header,(size_t)n,&info);
     printf("File header: %s (rc=%d)\n",
            rc==0?type_name(info.type):"Invalid",rc);
    }else printf("File header: unrecognized\n");
   }else printf("Cannot inspect short/invalid file\n");
  }
  fclose(f);
 } else printf("No EBOOT.PBP found.\n");
 printf("\nA: scene | Y: triangles | START: exit\n");
 int show_scene=0;
 int show_triangles=0;
 int gpu_ready=0;
#ifdef PSP3DS_SCENE_AUTOSTART
 gpu_ready=psp_3ds_scene_init()==0;
 show_scene=gpu_ready;
#endif
 uint32_t scene_frame=0;
 while(aptMainLoop()){
  hidScanInput();
  u32 pressed=hidKeysDown();
  circlePosition circle={0};
  hidCircleRead(&circle);
  PspNintendoCtrl psp_pad;
  if(psp_nintendo_ctrl_sample(target,hidKeysHeld(),circle.dx,circle.dy,
                             &psp_pad)==0 && (pressed&KEY_SELECT)) {
   printf("PSP pad: %04lx X:%u Y:%u\n",
          (unsigned long)psp_pad.buttons,(unsigned)psp_pad.lx,
          (unsigned)psp_pad.ly);
  }
  if(pressed&KEY_START)break;
  if(pressed&KEY_A){
   if(!gpu_ready)gpu_ready=psp_3ds_scene_init()==0;
   if(gpu_ready){show_scene=!show_scene;if(show_scene)show_triangles=0;}
  }
  if(pressed&KEY_Y){
   if(!gpu_ready)gpu_ready=psp_3ds_scene_init()==0;
   if(gpu_ready){show_triangles=!show_triangles;if(show_triangles)show_scene=0;}
  }
  if(gpu_ready &&
     ((show_triangles && psp_3ds_triangle_draw(scene_frame++,target)==0) ||
      (show_scene && psp_3ds_scene_draw(scene_frame++)==0))) {
   /* Citro3D submitted the synthetic 2D or triangle scene to PICA200. */
  } else {
   psp3ds_draw_boot_marker();
   gfxFlushBuffers();
   gfxSwapBuffers();
   gspWaitForVBlank();
  }
 }
 psp_3ds_scene_fini();
 gfxExit();if(high_clock)osSetSpeedupEnable(false);return 0;
}
