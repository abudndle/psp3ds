#include "psp_guarded_prx.h"
#include <string.h>
#define PT_LOAD 1u
static uint16_t r16(const uint8_t *p){
 return (uint16_t)(p[0]|(uint16_t)p[1]<<8);
}
static uint32_t r32(const uint8_t *p){
 return (uint32_t)p[0]|(uint32_t)p[1]<<8|
        (uint32_t)p[2]<<16|(uint32_t)p[3]<<24;
}
int psp_guarded_prx_load(PspMemoryGuard *guard,const uint8_t *elf,
                         size_t len,uint32_t base,PspGuardedPrx *out){
 if(!guard||!elf||!out||out->guest_ram||len<52u||(base&3u))
  return -1;
 memset(out,0,sizeof(*out));
 if(memcmp(elf,"\177ELF",4)!=0||elf[4]!=1||elf[5]!=1||elf[6]!=1||
    r16(elf+16)!=0xffa0u||r16(elf+18)!=8u)return -2;
 const uint32_t phoff=r32(elf+28);
 const uint32_t stride=r16(elf+42);
 const uint32_t count=r16(elf+44);
 if(!count||count>128u||stride<32u ||
    (uint64_t)phoff+(uint64_t)stride*count>len)return -2;
 uint64_t required=0;
 unsigned found=0;
 for(uint32_t i=0;i<count;i++){
  const uint8_t *ph=elf+phoff+(size_t)i*stride;
  if(r32(ph)!=PT_LOAD)continue;
  const uint64_t at=r32(ph+8),bytes=r32(ph+20);
  if(!bytes || (uint64_t)r32(ph+4)+r32(ph+16)>len ||
     r32(ph+16)>bytes || at+bytes>PSP_GUARDED_PRX_IMAGE_LIMIT)
   return -2;
  if(at+bytes>required)required=at+bytes;
  found++;
 }
 if(!found||!required)return -2;
 const uint64_t rounded=(required+4095u)&~UINT64_C(4095);
 if(rounded>PSP_GUARDED_PRX_IMAGE_LIMIT || (uint64_t)base+rounded>UINT32_MAX)
  return -2;
 void *guest=NULL;
 const int acquired=psp_memory_guard_acquire(guard,PSP_MEMORY_GUEST,
                                             (size_t)rounded,&guest);
 if(acquired!=0)return -3;
 memset(guest,0,(size_t)rounded);
 PspCpu cpu;
 psp_cpu_init(&cpu,(uint8_t*)guest,(size_t)rounded,base,base);
 PspPrxImageResult image;
 if(psp_prx_image_map_fresh(elf,len,&cpu,&image)!=PSP_PRX_IMAGE_OK){
  psp_memory_guard_release(guard,guest);
  return -4;
 }
 out->cpu=cpu;
 out->image=image;
 out->guest_ram=(uint8_t*)guest;
 out->allocated_bytes=(uint32_t)rounded;
 return 0;
}
int psp_guarded_prx_unload(PspMemoryGuard *guard,PspGuardedPrx *image){
 if(!guard||!image||!image->guest_ram)return -1;
 if(psp_memory_guard_release(guard,image->guest_ram)!=0)return -2;
 memset(image,0,sizeof(*image));
 return 0;
}
