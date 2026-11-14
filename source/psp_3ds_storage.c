#include "psp_3ds_storage.h"
#include <string.h>
int psp_3ds_storage_init(Psp3dsStorage *s,uint64_t bytes,
                         Psp3dsStorageRead read,void *opaque){
 if(s)memset(s,0,sizeof(*s));
 if(!s || !read || !bytes || bytes>PSP_3DS_SD_FAT32_MAX_FILE)
  return -1;
 s->read=read;s->opaque=opaque;s->length=bytes;
 return 0;
}
int psp_3ds_storage_read(Psp3dsStorage *s,uint64_t offset,
                         uint8_t *dst,size_t bytes){
 if(!s||!s->read||!dst||!bytes||bytes>PSP_3DS_SD_MAX_CHUNK||
    offset>s->length || bytes>s->length-offset)return -1;
 /* Read through private staging so a partial/error FS service read cannot
  * corrupt a caller's data. Even at 4GiB file offsets no RAM proportional
  * to SD volume or the file size is needed. */
 uint8_t staging[PSP_3DS_SD_MAX_CHUNK];
 if(s->read(s->opaque,offset,staging,bytes)!=0)return -1;
 memcpy(dst,staging,bytes);
 s->calls++;
 s->read_bytes+=(uint32_t)bytes; /* diagnostic may wrap; no sizes inferred */
 return 0;
}
