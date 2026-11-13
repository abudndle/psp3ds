#include "psp_iso.h"
#include <limits.h>
#include <string.h>
int psp_iso_open(PspIso *iso,const char *path){
 if(!iso || !path)return -1;
 memset(iso,0,sizeof(*iso));
 FILE *file=fopen(path,"rb");if(!file)return -1;
 if(fseek(file,0,SEEK_END)!=0){fclose(file);return -1;}
 long size=ftell(file);
 if(size<2048 || size%2048!=0 || (unsigned long long)size/2048>UINT32_MAX){fclose(file);return -1;}
 /* A minimal check for a standard ISO9660 primary volume descriptor. */
 if(size < 18*2048L || fseek(file,16*2048L+1,SEEK_SET)!=0){fclose(file);return -1;}
 unsigned char signature[5];
 if(fread(signature,1,5,file)!=5 || memcmp(signature,"CD001",5)!=0){fclose(file);return -1;}
 iso->file=file;iso->sectors=(uint32_t)(size/2048);return 0;
}
void psp_iso_close(PspIso *iso){if(iso){if(iso->file)fclose(iso->file);iso->file=NULL;iso->sectors=0;}}
int psp_iso_read(void *ctx,uint32_t lba,uint8_t *out){
 PspIso *iso=(PspIso *)ctx;
 if(!iso || !iso->file || !out || lba>=iso->sectors)return -1;
 unsigned long long offset=(unsigned long long)lba*2048u;
 if(offset>LONG_MAX || fseek(iso->file,(long)offset,SEEK_SET)!=0)return -1;
 return fread(out,1,2048,iso->file)==2048?0:-1;
}
int psp_iso_cached(PspIso *iso,PspPerf *perf,uint32_t lba,uint8_t *out){
 if(!iso || !perf || lba>=iso->sectors)return -1;
 return psp_perf_read_sector(perf,lba,psp_iso_read,iso,out);
}
