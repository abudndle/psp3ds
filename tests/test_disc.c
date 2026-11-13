#include "psp_disc.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#define S 2048
static uint8_t image[32][S];
static void w32(uint8_t *p,uint32_t n){for(int i=0;i<4;i++)p[i]=(uint8_t)(n>>(8*i));}
static int read_sector(void *ctx,uint32_t lba,uint8_t *out){(void)ctx;if(lba>=32)return -1;memcpy(out,image[lba],S);return 0;}
static size_t record(uint8_t *p,uint32_t lba,uint32_t size,const char *name,uint8_t isdir){
 size_t n=strlen(name);size_t len=33+n+(n%2==0?1:0);
 assert(len<256);memset(p,0,len);p[0]=(uint8_t)len;w32(p+2,lba);w32(p+10,size);p[25]=isdir?2:0;p[28]=1;p[31]=1;p[32]=(uint8_t)n;memcpy(p+33,name,n);return len;
}
int main(void){
 memset(image,0,sizeof image);
 uint8_t *pvd=image[16];pvd[0]=1;memcpy(pvd+1,"CD001",5);pvd[6]=1;
 uint8_t *root=pvd+156;root[0]=34;w32(root+2,18);w32(root+10,2048);root[25]=2;
 size_t n=record(image[18],19,2048,"PSP_GAME",1);
 assert(n<2048);
 n=record(image[19],21,60,"PARAM.SFO;1",0);
 n+=record(image[19]+n,20,2048,"SYSDIR",1);
 n=record(image[20],22,5000,"EBOOT.BIN;1",0);
 (void)n;
 memcpy(image[21],"\0PSF",4);memcpy(image[22],"~PSP",4);
 PspDiscEntry found;
 assert(psp_disc_find(read_sector,NULL,"/PSP_GAME/PARAM.SFO",&found)==0);
 assert(found.lba==21 && found.size==60 && !found.is_dir);
 uint8_t output[64];assert(psp_disc_read(read_sector,NULL,found,0,output,4)==0);
 assert(!memcmp(output,"\0PSF",4));
 assert(psp_disc_find(read_sector,NULL,"/PSP_GAME/SYSDIR/EBOOT.BIN",&found)==0);
 assert(found.lba==22 && found.size==5000);
 assert(psp_disc_read(read_sector,NULL,found,0,output,4)==0);
 assert(!memcmp(output,"~PSP",4));
 assert(psp_disc_find(read_sector,NULL,"/PSP_GAME/SYSDIR/UNKNOWN",&found)<0);
 assert(psp_disc_find(read_sector,NULL,"/PSP_GAME/PARAM.SFO/BAD",&found)<0);
 assert(psp_disc_read(read_sector,NULL,found,5000,output,1)<0);
 assert(psp_disc_find(read_sector,NULL,"PSP_GAME",&found)<0);
 image[16][1]='X';assert(psp_disc_find(read_sector,NULL,"/PSP_GAME",&found)<0);
 puts("PASS: ISO9660 root-to-PSP_GAME traversal and bounded file reads");return 0;
}
