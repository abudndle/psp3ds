#include <3ds.h>
#include <stdio.h>
#include <stdlib.h>
#include "psp_core.h"
#include "psp_catalog.h"
#include "psp_platform_budget.h"
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
int main(void) {
 osSetSpeedupEnable(true); /* Request New 3DS high-speed mode; old models unaffected. */
 gfxInitDefault();consoleInit(GFX_TOP,NULL);
 printf("PSP3DS | research prototype\n");
 printf("PSP file inspection + basic MIPS interpreter\n");
 printf("Not a playable PSP emulator yet.\n\n");
 bool new_model=false;
 Result hardware=APT_CheckNew3DS(&new_model);
 PspPlatformBudget policy;
 PspTarget target=(R_SUCCEEDED(hardware) && new_model)?
    PSP_TARGET_NEW3DS:PSP_TARGET_OLD3DS;
 if(psp_platform_budget_choose(target,&policy)==0 &&
    psp_platform_budget_validate(&policy)==0){
  printf("Policy: %s\n",policy.name);
  printf("Hot cap %luK, texture cap %luK\n",
    (unsigned long)(policy.hot_ram_budget/1024u),
    (unsigned long)(policy.texture_budget/1024u));
  printf("Sprite batching planned\n");
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
 printf("\nSTART: exit\n");
 while(aptMainLoop()){
  hidScanInput();if(hidKeysDown()&KEY_START)break;
  psp3ds_draw_boot_marker();
  gfxFlushBuffers();
  gfxSwapBuffers();
  gspWaitForVBlank();
 }
 gfxExit();osSetSpeedupEnable(false);return 0;
}
