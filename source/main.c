#include <3ds.h>
#include <stdio.h>
#include <stdlib.h>
#include "psp_core.h"
#include "psp_catalog.h"
static const char *type_name(PspFileType t) {
 switch(t) {case PSP_FILE_PBP:return "PBP";case PSP_FILE_ELF:return "MIPS ELF";case PSP_FILE_ISO:return "ISO";case PSP_FILE_CSO:return "CSO";default:return "Unknown";}
}
int main(void) {
 osSetSpeedupEnable(true); /* Request New 3DS high-speed mode; old models unaffected. */
 gfxInitDefault();consoleInit(GFX_TOP,NULL);
 printf("PSP3DS | research prototype\n");
 printf("PSP file inspection + basic MIPS interpreter\n");
 printf("Not a playable PSP emulator yet.\n\n");
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
 while(aptMainLoop()){hidScanInput();if(hidKeysDown()&KEY_START)break;gspWaitForVBlank();}
 gfxExit();osSetSpeedupEnable(false);return 0;
}
