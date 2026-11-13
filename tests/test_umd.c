#include "psp_umd.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define S 2048
static void put32(uint8_t *p,uint32_t x){for(int i=0;i<4;i++)p[i]=(uint8_t)(x>>(i*8));}
static void put64(uint8_t *p,uint64_t x){put32(p,(uint32_t)x);put32(p+4,(uint32_t)(x>>32));}
static void volume(uint8_t sector[S],unsigned char marker){
 memset(sector,0,S);sector[0]=1;memcpy(sector+1,"CD001",5);sector[6]=1;
 sector[156]=34;put32(sector+158,18);put32(sector+166,S);
 sector[181]=2;sector[188]=1;sector[189]=1;sector[200]=marker;
}
static void create_iso(const char *name,uint8_t marker){
 FILE *f=fopen(name,"wb");assert(f);uint8_t sector[S];
 for(unsigned lba=0;lba<20;lba++){
  memset(sector,marker,S);
  if(lba==16)volume(sector,marker);
  if(lba==18){
   memset(sector,0,S);sector[0]=42;put32(sector+2,19);put32(sector+10,9);
   sector[28]=1;sector[31]=1;sector[32]=8;memcpy(sector+33,"TEST.TXT",8);
  }
  if(lba==19){memset(sector,0,S);memcpy(sector,"GAME ",5);sector[5]=marker;}
  assert(fwrite(sector,1,S,f)==S);
 }
 assert(fclose(f)==0);
}
static void create_cso_plain(const char *name, const char *iso_path){
 FILE *f=fopen(name,"wb"),*iso=fopen(iso_path,"rb");assert(f&&iso);
 uint8_t h[24]={0};memcpy(h,"CISO",4);put32(h+4,24);put64(h+8,20*S);
 put32(h+16,S);h[20]=1;assert(fwrite(h,1,24,f)==24);
 const unsigned start=24+21*4;
 for(unsigned i=0;i<=20;i++){
  uint8_t index[4];put32(index,start+i*S|(i==20?0:0x80000000u));
  assert(fwrite(index,1,4,f)==4);
 }
 uint8_t buf[S];for(unsigned i=0;i<20;i++){
  assert(fread(buf,1,S,iso)==S);assert(fwrite(buf,1,S,f)==S);
 }
 assert(fclose(iso)==0&&fclose(f)==0);
}
int main(void){
 const char *a="test_umd_a.iso",*b="test_umd_b.iso",*c="test_umd_c.cso";
 create_iso(a,'A');create_iso(b,'B');create_cso_plain(c,a);
 PspUmd *dev=calloc(1,sizeof(*dev));assert(dev);psp_umd_init(dev);
 uint8_t out[S];PspDiscEntry entry;
 assert(psp_umd_sector(dev,16,out)<0);
 assert(psp_umd_mount(dev,a)==0&&dev->kind==PSP_UMD_ISO);
 assert(psp_umd_sector(dev,16,out)==1&&out[200]=='A');
 assert(psp_umd_find(dev,"/TEST.TXT",&entry)==0&&entry.size==9);
 uint8_t text[9];assert(psp_umd_read(dev,entry,0,text,9)==0);
 assert(!memcmp(text,"GAME A",6));
 assert(psp_umd_read(dev,entry,1,text,9)<0);
 assert(psp_umd_mount(dev,b)==0&&dev->kind==PSP_UMD_ISO);
 assert(psp_umd_sector(dev,16,out)==1&&out[200]=='B');
 assert(psp_umd_find(dev,"/TEST.TXT",&entry)==0);
 assert(psp_umd_read(dev,entry,0,text,9)==0&&!memcmp(text,"GAME B",6));
 assert(psp_umd_mount(dev,c)==0&&dev->kind==PSP_UMD_CSO);
 assert(psp_umd_sector(dev,16,out)==1&&out[200]=='A');
 assert(psp_umd_find(dev,"/TEST.TXT",&entry)==0);
 assert(psp_umd_read(dev,entry,0,text,9)==0&&!memcmp(text,"GAME A",6));
 assert(psp_umd_sector(dev,20,out)<0);
 psp_umd_unmount(dev);assert(psp_umd_sector(dev,16,out)<0);
 assert(psp_umd_mount(dev,"missing_umd_file")<0&&dev->kind==PSP_UMD_NONE);
 assert(psp_umd_find(dev,"/TEST.TXT",&entry)<0);
 free(dev);remove(a);remove(b);remove(c);
 puts("PASS: unified ISO/CSO mount, sector caching, ISO9660 files, media replacement");return 0;
}
