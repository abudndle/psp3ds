#include <3ds.h>
#include <stdio.h>
#include <stdlib.h>
#include "psp_core.h"
#include "psp_catalog.h"
#include "psp_platform_budget.h"
#include "psp_video_layout.h"
#include "psp_native_texture.h"
#include "psp_nintendo_input.h"
#include "psp_features.h"
#include "psp_scene.h"
int psp_3ds_scene_init(void);
int psp_3ds_scene_draw(uint32_t frame);
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
 osSetSpeedupEnable(true); /* Request New 3DS high-speed mode; old models unaffected. */
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
 PspPlatformBudget policy;
 PspTarget target=(R_SUCCEEDED(hardware) && new_model)?
    PSP_TARGET_NEW3DS:PSP_TARGET_OLD3DS;
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
   if(n>=0 && n<=8*1024*1024 && fseek(f,0,SEEK_SET)==0) {
    unsigned char *p=malloc((size_t)n?(size_t)n:1);
    if(p) { if(fread(p,1,(size_t)n,f)==(size_t)n){PspFileInfo info;int rc=psp_inspect(p,(size_t)n,&info);printf("File: %s (rc=%d)\n",rc==0?type_name(info.type):"Invalid",rc);}free(p); }
   } else printf("File too large for prototype scan.\n");
  }
  fclose(f);
 } else printf("No EBOOT.PBP found.\n");
 printf("\nA: synthetic scene | START: exit\n");
 int show_scene=0;
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
   if(gpu_ready)show_scene=!show_scene;
  }
  if(show_scene && gpu_ready &&
     psp_3ds_scene_draw(scene_frame++)==0) {
   /* Citro3D submitted the scene to the bottom GPU target. */
  } else {
   psp3ds_draw_boot_marker();
   gfxFlushBuffers();
   gfxSwapBuffers();
   gspWaitForVBlank();
  }
 }
 psp_3ds_scene_fini();
 gfxExit();osSetSpeedupEnable(false);return 0;
}
