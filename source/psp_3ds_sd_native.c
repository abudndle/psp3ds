#include <3ds.h>
#include <stdint.h>
#include <stdio.h>
#include "psp_3ds_storage.h"

/* 64-bit offsets matter for 3DS SD files >2 GiB; libc fseek(long)
 * is not the API used here. This diagnostic needs an optional,
 * user-created SD file. It never exposes file contents or executes them. */
typedef struct {Handle handle;} NativeSdFile;
static int sd_chunk(void *opaque,uint64_t offset,uint8_t *dst,size_t length){
 NativeSdFile *file=(NativeSdFile*)opaque;
 u32 got=0;
 if(!file||!file->handle||!dst||!length||
    length>PSP_3DS_SD_MAX_CHUNK)return -1;
 Result rc=FSFILE_Read(file->handle,&got,(u64)offset,dst,(u32)length);
 return R_SUCCEEDED(rc) && got==length?0:-1;
}
int psp_3ds_native_sd_probe(void){
 Result init=fsInit();
 if(R_FAILED(init)){
  printf("SD storage service unavailable\n");
  return -1;
 }
 const char *files[]={"/psp3ds/stream-test.bin","/psp3ds/EBOOT.PBP"};
 NativeSdFile file={0};
 unsigned selected=2u;
 for(unsigned i=0;i<2;i++){
  Result rc=FSUSER_OpenFileDirectly(&file.handle,ARCHIVE_SDMC,
       fsMakePath(PATH_EMPTY,""),fsMakePath(PATH_ASCII,files[i]),
       FS_OPEN_READ,0u);
  if(R_SUCCEEDED(rc)){selected=i;break;}
  file.handle=0;
 }
 if(selected==2u){
  printf("SD 64-bit read: optional file absent\n");
  fsExit();
  return 1;
 }
 u64 size=0;
 const Result rc=FSFILE_GetSize(file.handle,&size);
 int status=-1;
 if(R_SUCCEEDED(rc) && size && size<=PSP_3DS_SD_FAT32_MAX_FILE){
  Psp3dsStorage stream;
  if(psp_3ds_storage_init(&stream,(uint64_t)size,sd_chunk,&file)==0){
   uint8_t sample[16];
   const size_t length=size<sizeof sample?(size_t)size:sizeof sample;
   status=psp_3ds_storage_read(&stream,0u,sample,length)==0 &&
          psp_3ds_storage_read(&stream,(uint64_t)size-length,
                               sample,length)==0?0:-1;
  }
 }
 printf("SD file stream: %s\n",status==0?"PASS (real read)":"FAILED");
 printf("File size: %lu MiB (not guest RAM)\n",
        (unsigned long)(size/(1024u*1024u)));
 printf("3DS FS u64 read; no PSP code run\n");
 FSFILE_Close(file.handle);
 svcCloseHandle(file.handle);
 fsExit();
 return status;
}
