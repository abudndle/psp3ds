#include "psp_cso.h"
#include "psp_iso.h"
#include "psp_disc.h"
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
typedef struct {PspCso cso;PspIso iso;int cso_active;} Image;
static int read_sector(void *ctx,uint32_t lba,uint8_t *out){
 Image *i=ctx;return i->cso_active?psp_cso_read(&i->cso,lba,out):psp_iso_read(&i->iso,lba,out);
}
static uint16_t u16(const unsigned char *p){return (uint16_t)(p[0]|(uint16_t)p[1]<<8);}
static uint32_t u32(const unsigned char *p){return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;}
static void show_sfo(const uint8_t *buf,size_t sz){
 if(sz<20 || memcmp(buf,"\0PSF",4))return;
 uint32_t key=u32(buf+8),value=u32(buf+12),count=u32(buf+16);
 if(count>128 || (uint64_t)20+count*16>sz)return;
 for(uint32_t i=0;i<count;i++){
  const uint8_t *r=buf+20+16*i;uint32_t k=u16(r),size=u32(r+4),v=u32(r+12);
  if((uint64_t)key+k>=sz || (uint64_t)value+v+size>sz)continue;
  const char *name=(const char *)buf+key+k;
  if(!memchr(name,0,sz-key-k))continue;
  if(!strcmp(name,"TITLE") || !strcmp(name,"DISC_ID") || !strcmp(name,"PSP_SYSTEM_VER") || !strcmp(name,"CATEGORY")){
   size_t n=size<100?size:100;
   char str[101];memcpy(str,buf+value+v,n);str[n]=0;
   for(size_t j=0;j<n;j++)if(str[j]=='\n' || str[j]=='\r')str[j]=' ';
   printf("%s: %s\n",name,str);
  }
 }
}
int main(int argc,char **argv){
 if(argc!=2){fprintf(stderr,"usage: psp_disc_probe <game.cso|game.iso>\n");return 2;}
 Image *img=calloc(1,sizeof(*img));if(!img)return 2;
 if(psp_cso_open(&img->cso,argv[1])==0)img->cso_active=1;
 else if(psp_iso_open(&img->iso,argv[1])==0)img->cso_active=0;
 else {fprintf(stderr,"Unsupported/invalid ISO or CSO\n");free(img);return 1;}
 printf("Image: %s\nSectors: %u\n",img->cso_active?"CSO v1":"ISO",img->cso_active?img->cso.sectors:img->iso.sectors);
 PspDiscEntry entry;
 if(psp_disc_find(read_sector,img,"/PSP_GAME/PARAM.SFO",&entry)!=0){fprintf(stderr,"PARAM.SFO not found\n");goto error;}
 if(entry.size>65536){fprintf(stderr,"PARAM.SFO too large\n");goto error;}
 uint8_t *sfo=malloc(entry.size);if(!sfo)goto error;
 if(psp_disc_read(read_sector,img,entry,0,sfo,entry.size)!=0){free(sfo);goto error;}
 show_sfo(sfo,entry.size);free(sfo);
 if(psp_disc_find(read_sector,img,"/PSP_GAME/SYSDIR/EBOOT.BIN",&entry)!=0)goto error;
 uint8_t magic[4];if(psp_disc_read(read_sector,img,entry,0,magic,4)!=0)goto error;
 printf("EBOOT.BIN: %u bytes, signature %02X %02X %02X %02X\n",entry.size,magic[0],magic[1],magic[2],magic[3]);
 if(!memcmp(magic,"~PSP",4))printf("Blocker: encrypted/signed PSP executable is not supported by PSP3DS.\n");
 else if(!memcmp(magic,"\177ELF",4))printf("Plain ELF header; CPU and kernel support still incomplete.\n");
 else printf("Unknown/unsupported executable signature.\n");
 if(img->cso_active)psp_cso_close(&img->cso);else psp_iso_close(&img->iso);free(img);return 0;
error:
 if(img->cso_active)psp_cso_close(&img->cso);else psp_iso_close(&img->iso);free(img);return 1;
}
