#include "psp_umd.h"
#include <string.h>
void psp_umd_init(PspUmd *d){
 if(!d)return;
 memset(d,0,sizeof(*d));
 psp_perf_init(&d->cache);
}
void psp_umd_unmount(PspUmd *d){
 if(!d)return;
 if(d->kind==PSP_UMD_CSO)psp_cso_close(&d->cso);
 if(d->kind==PSP_UMD_ISO)psp_iso_close(&d->iso);
 d->kind=PSP_UMD_NONE;
 d->generation++;
 psp_perf_media_changed(&d->cache);
}
static int disk_reader(void *ctx,uint32_t lba,uint8_t *out){
 PspUmd *d=(PspUmd*)ctx;
 if(!d || !out)return -1;
 if(d->kind==PSP_UMD_CSO)return psp_cso_read(&d->cso,lba,out);
 if(d->kind==PSP_UMD_ISO)return psp_iso_read(&d->iso,lba,out);
 return -1;
}
static int cached_reader(void *ctx,uint32_t lba,uint8_t *out){
 int r=psp_umd_sector((PspUmd*)ctx,lba,out);
 return r>=0?0:-1;
}
int psp_umd_sector(PspUmd *d,uint32_t lba,uint8_t *out){
 if(!d || !out || d->kind==PSP_UMD_NONE)return -1;
 uint32_t sectors=d->kind==PSP_UMD_CSO?d->cso.sectors:d->iso.sectors;
 if(lba>=sectors)return -1;
 return psp_perf_read_sector(&d->cache,lba,disk_reader,d,out);
}
int psp_umd_mount(PspUmd *d,const char *path){
 if(!d || !path)return -1;
 psp_umd_unmount(d);
 FILE *f=fopen(path,"rb");
 if(!f)return -1;
 uint8_t magic[4];size_t got=fread(magic,1,4,f);fclose(f);
 if(got!=4)return -1;
 if(memcmp(magic,"CISO",4)==0){
  if(psp_cso_open(&d->cso,path)!=0)return -1;
  d->kind=PSP_UMD_CSO;
 }else{
  if(psp_iso_open(&d->iso,path)!=0)return -1;
  d->kind=PSP_UMD_ISO;
 }
 uint8_t pvd[PSP_PERF_SECTOR_SIZE];
 if(psp_umd_sector(d,16,pvd)<0 || pvd[0]!=1 ||
    memcmp(pvd+1,"CD001",5)!=0 || pvd[6]!=1){
  psp_umd_unmount(d);return -1;
 }
 return 0;
}
int psp_umd_find(PspUmd *d,const char *path,PspDiscEntry *entry){
 if(!d || d->kind==PSP_UMD_NONE)return -1;
 return psp_disc_find(cached_reader,d,path,entry);
}
int psp_umd_read(PspUmd *d,PspDiscEntry entry,uint32_t off,void *dst,size_t n){
 if(!d || d->kind==PSP_UMD_NONE)return -1;
 return psp_disc_read(cached_reader,d,entry,off,dst,n);
}
