#include "psp_disc.h"
#include <string.h>
#include <stdint.h>
#define SECTOR 2048u
#define MAX_DIR_BYTES (1024u*1024u)
static uint32_t u32(const uint8_t *p) {
 return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;
}
static int directory(PspSectorRead reader,void *ctx,PspDiscEntry dir,
                     const char *name,size_t name_len,PspDiscEntry *out){
 if(!dir.is_dir || !dir.size || dir.size>MAX_DIR_BYTES || !name_len)return -1;
 uint8_t b[SECTOR];
 for(uint32_t offset=0;offset<dir.size;){
  if(offset%SECTOR==0 && reader(ctx,dir.lba+offset/SECTOR,b)!=0)return -1;
  unsigned index=offset%SECTOR,len=b[index];
  if(len==0){offset=(offset/SECTOR+1)*SECTOR;continue;}
  if(len<34 || len>SECTOR-index || len>dir.size-offset)return -1;
  const uint8_t *r=b+index;unsigned size=r[32];
  if((unsigned)33+size>len)return -1;
  if(size==name_len && !memcmp(r+33,name,name_len)){
   out->lba=u32(r+2);out->size=u32(r+10);out->is_dir=(r[25]&2)?1:0;
   if(!out->size || out->lba>UINT32_MAX-(out->size-1)/SECTOR)return -1;
   return 0;
  }
  /* Match ISO9660 version suffix, e.g. EBOOT.BIN;1. */
  if(size==name_len+2 && r[33+name_len]==';' && r[34+name_len]=='1' &&
     !memcmp(r+33,name,name_len)){
   out->lba=u32(r+2);out->size=u32(r+10);out->is_dir=(r[25]&2)?1:0;
   if(!out->size || out->lba>UINT32_MAX-(out->size-1)/SECTOR)return -1;
   return 0;
  }
  offset+=len;
 }
 return -2;
}
int psp_disc_find(PspSectorRead reader,void *ctx,const char *path,PspDiscEntry *out){
 if(!reader || !path || !out || path[0]!='/')return -1;
 uint8_t pvd[SECTOR];
 if(reader(ctx,16,pvd)!=0 || pvd[0]!=1 || memcmp(pvd+1,"CD001",5) || pvd[6]!=1)return -1;
 const uint8_t *root=pvd+156;
 if(root[0]<34 || !(root[25]&2))return -1;
 PspDiscEntry current={u32(root+2),u32(root+10),1};
 if(!current.size || current.size>MAX_DIR_BYTES)return -1;
 const char *cursor=path+1;
 if(!*cursor)return -1;
 while(*cursor){
  const char *end=cursor;
  while(*end && *end!='/')end++;
  size_t len=(size_t)(end-cursor);
  if(!len || len>64)return -1;
  PspDiscEntry next;
  if(directory(reader,ctx,current,cursor,len,&next)!=0)return -2;
  current=next;
  if(!*end)break;
  if(!current.is_dir)return -2;
  cursor=end+1;
  if(!*cursor)return -1;
 }
 *out=current;return 0;
}
int psp_disc_read(PspSectorRead reader,void *ctx,PspDiscEntry entry,
                  uint32_t offset,void *dst,size_t len){
 if(!reader || (!dst && len) || entry.is_dir || offset>entry.size || len>entry.size-offset)return -1;
 uint8_t sector[SECTOR];uint8_t *p=(uint8_t*)dst;
 while(len){
  uint32_t sector_number=offset/SECTOR;
  if(entry.lba>UINT32_MAX-sector_number || reader(ctx,entry.lba+sector_number,sector)!=0)return -1;
  size_t skip=offset%SECTOR,amount=SECTOR-skip;
  if(amount>len)amount=len;
  memcpy(p,sector+skip,amount);
  p+=amount;offset+=(uint32_t)amount;len-=amount;
 }
 return 0;
}
