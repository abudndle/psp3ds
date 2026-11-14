#include "psp_3ds_storage.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
typedef struct {
 uint64_t last_offset;
 uint32_t calls;
 uint32_t fail;
} Mock;
static uint8_t pattern(uint64_t address){
 return (uint8_t)((address>>12u)*17u+(address&255u));
}
static int source(void *opaque,uint64_t offset,uint8_t *dst,size_t len){
 Mock *m=(Mock*)opaque;
 if(!m||!dst||!len || len>PSP_3DS_SD_MAX_CHUNK)return -1;
 m->calls++;m->last_offset=offset;
 for(size_t i=0;i<len;i++)dst[i]=pattern(offset+i);
 return m->fail?-1:0;
}
int main(void){
 Mock mock={0};Psp3dsStorage s;
 assert(PSP_3DS_SD_OFFICIAL_MAX_BYTES==UINT64_C(34359738368));
 assert(PSP_3DS_SD_FAT32_MAX_FILE==UINT64_C(4294967295));
 assert(psp_3ds_storage_init(&s,PSP_3DS_SD_FAT32_MAX_FILE,source,&mock)==0);
 uint8_t a[32];
 const uint64_t far=UINT64_C(0xffffffff)-31u;
 assert(psp_3ds_storage_read(&s,far,a,sizeof a)==0);
 assert(mock.last_offset==far && mock.calls==1u);
 for(unsigned i=0;i<sizeof a;i++)assert(a[i]==pattern(far+i));
 assert(s.calls==1u && s.read_bytes==sizeof a);
 assert(psp_3ds_storage_read(&s,UINT64_C(0x80000004),a,4)==0);
 for(unsigned i=0;i<4;i++)assert(a[i]==pattern(UINT64_C(0x80000004)+i));
 assert(mock.last_offset>UINT64_C(0x80000000));
 assert(psp_3ds_storage_read(&s,UINT64_C(0xffffffff)-1u,a,2u)==-1);
 assert(psp_3ds_storage_read(&s,0,a,PSP_3DS_SD_MAX_CHUNK+1u)==-1);
 assert(psp_3ds_storage_read(&s,0,a,0)==-1);
 assert(psp_3ds_storage_read(&s,0,NULL,1)==-1);
 memset(a,0x5a,sizeof a);
 mock.fail=1u;
 assert(psp_3ds_storage_read(&s,far,a,sizeof a)==-1);
 for(unsigned i=0;i<sizeof a;i++)assert(a[i]==0x5au);
 assert(s.calls==2u); /* failed callback not counted as successful read */
 assert(psp_3ds_storage_init(&s,UINT64_C(0x100000000),source,&mock)==-1);
 assert(s.length==0u);
 assert(psp_3ds_storage_init(&s,0u,source,&mock)==-1);
 assert(psp_3ds_storage_init(&s,4u,NULL,&mock)==-1);
 assert(psp_3ds_storage_init(NULL,4u,source,&mock)==-1);
 puts("PASS: 3DS SD FAT32 per-file cap, 64-bit reads past 2GiB, partial-read atomicity");
 return 0;
}
