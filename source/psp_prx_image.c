#include "psp_prx_image.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define PRX_ELF_TYPE 0xffa0u
#define PRX_MAX_RAM (32u*1024u*1024u)
#define PRX_MAX_HEADERS 128u
#define PT_LOAD 1u
#define PT_SCE_PSPREL 0x700000a0u
#define PT_SCE_PSPREL2 0x700000a1u

static uint16_t u16(const uint8_t *p){
 return (uint16_t)(p[0]|(uint16_t)p[1]<<8);
}
static uint32_t u32(const uint8_t *p){
 return (uint32_t)p[0]|(uint32_t)p[1]<<8|
        (uint32_t)p[2]<<16|(uint32_t)p[3]<<24;
}
static int overlap(const uint8_t *a,size_t as,const uint8_t *b,size_t bs){
 const uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;
 if(as>UINTPTR_MAX-x || bs>UINTPTR_MAX-y)return 1;
 return x<y+bs && y<x+as;
}

static int prx_map_core(const uint8_t *elf,size_t len,
                        PspCpu *cpu,PspPrxImageResult *out,
                        int fresh_scratch){
 if(out)memset(out,0,sizeof(*out));
 if(!elf||!cpu||!out||!cpu->ram||!cpu->ram_size||
    cpu->ram_size>PRX_MAX_RAM||len<52u||
    overlap(elf,len,cpu->ram,cpu->ram_size))
  return PSP_PRX_IMAGE_ARGUMENT;
 if(memcmp(elf,"\177ELF",4)!=0 || elf[4]!=1 || elf[5]!=1 ||
    elf[6]!=1 || u16(elf+16)!=PRX_ELF_TYPE ||
    u16(elf+18)!=8u || u16(elf+40)<52u)
  return PSP_PRX_IMAGE_FORMAT;
 const uint32_t entry=u32(elf+24);
 const uint32_t phoff=u32(elf+28);
 const uint32_t phsize=u16(elf+42),count=u16(elf+44);
 if(!count||count>PRX_MAX_HEADERS||phsize<32u||
    (uint64_t)phoff+(uint64_t)phsize*count>len||(entry&3u))
  return PSP_PRX_IMAGE_FORMAT;
 if((uint64_t)cpu->ram_base+cpu->ram_size>UINT32_MAX)
  return PSP_PRX_IMAGE_MAPPING;
 PspRelocSegment mapped[PRX_MAX_HEADERS];
 memset(mapped,0,sizeof(mapped));
 size_t used=0;
 uint64_t high=0;
 unsigned streams=0,entry_found=0;
 for(unsigned i=0;i<count;i++){
  const uint8_t *ph=elf+phoff+(size_t)i*phsize;
  const uint32_t type=u32(ph);
  if(type==PT_SCE_PSPREL||type==PT_SCE_PSPREL2){
   streams++;
   continue;
  }
  if(type!=PT_LOAD)continue;
  const uint32_t off=u32(ph+4),original=u32(ph+8);
  const uint32_t filesz=u32(ph+16),memsz=u32(ph+20),flags=u32(ph+24);
  if(!memsz||filesz>memsz||(uint64_t)off+filesz>len||
     (uint64_t)original+memsz>cpu->ram_size||
     (uint64_t)cpu->ram_base+original+memsz>UINT32_MAX)
   return PSP_PRX_IMAGE_MAPPING;
  for(unsigned j=0;j<i;j++){
   if(!mapped[j].mem_size)continue;
   const uint64_t old_start=mapped[j].original_vaddr;
   if(original<old_start+mapped[j].mem_size &&
      old_start<(uint64_t)original+memsz)
    return PSP_PRX_IMAGE_MAPPING;
  }
  mapped[i].original_vaddr=original;
  mapped[i].mapped_vaddr=cpu->ram_base+original;
  mapped[i].mem_size=memsz;
  used++;
  if((uint64_t)original+memsz>high)high=(uint64_t)original+memsz;
  if((flags&1u)&&entry>=original&&(uint64_t)entry+4u<=
     (uint64_t)original+filesz)entry_found=1;
 }
 if(!used||!entry_found||!streams||streams>16u)
  return PSP_PRX_IMAGE_MAPPING;
 /* Only bytes in the mapped image may be modified. Preserve gaps too.
  * The source image remains external; on allocation failure no guest
  * state changes. Limit snapshot to the image span rather than 32 MiB. */
 const size_t backup_len=(size_t)high;
 uint8_t *backup=NULL;
 if(!fresh_scratch){
  backup=(uint8_t*)malloc(backup_len);
  if(!backup)return PSP_PRX_IMAGE_NO_MEMORY;
  memcpy(backup,cpu->ram,backup_len);
 }
 for(unsigned i=0;i<count;i++){
  if(!mapped[i].mem_size)continue;
  const uint8_t *ph=elf+phoff+(size_t)i*phsize;
  const size_t off=mapped[i].original_vaddr;
  memset(cpu->ram+off,0,mapped[i].mem_size);
  memcpy(cpu->ram+off,elf+u32(ph+4),u32(ph+16));
 }
 PspCpu candidate=*cpu;
 size_t relocated_streams=0;
 const int reloc=fresh_scratch?
   psp_reloc_image_apply_scratch(elf,len,&candidate,mapped,count,
                                 &relocated_streams):
   psp_reloc_image_apply(elf,len,&candidate,mapped,count,
                         &relocated_streams);
 if(reloc!=0 || relocated_streams!=streams){
  if(backup)memcpy(cpu->ram,backup,backup_len);
  free(backup);
  return PSP_PRX_IMAGE_RELOCATION;
 }
 free(backup);
 const uint32_t guest_entry=cpu->ram_base+entry;
 psp_cpu_init(&candidate,cpu->ram,cpu->ram_size,cpu->ram_base,guest_entry);
 *cpu=candidate;
 out->entry=guest_entry;
 out->image_bytes=(uint32_t)high;
 out->load_segments=(uint16_t)used;
 out->relocation_streams=(uint16_t)streams;
 return PSP_PRX_IMAGE_OK;
}

int psp_prx_image_map_relocated(const uint8_t *elf,size_t elf_size,
                                PspCpu *cpu,PspPrxImageResult *out){
 return prx_map_core(elf,elf_size,cpu,out,0);
}
int psp_prx_image_map_fresh(const uint8_t *elf,size_t elf_size,
                             PspCpu *cpu,PspPrxImageResult *out){
 return prx_map_core(elf,elf_size,cpu,out,1);
}
