#include "psp_reloc_image.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define PT_LOAD 1u
#define PT_SCE_PSPREL 0x700000a0u
#define PT_SCE_PSPREL2 0x700000a1u
#define MAX_HEADERS 128u
#define MAX_STREAMS 16u
#define MAX_RAM_SNAPSHOT (32u*1024u*1024u)

static uint16_t rd16(const uint8_t *p) {
 return (uint16_t)(p[0] | (uint16_t)p[1]<<8);
}
static uint32_t rd32(const uint8_t *p) {
 return (uint32_t)p[0] | (uint32_t)p[1]<<8 |
        (uint32_t)p[2]<<16 | (uint32_t)p[3]<<24;
}
int psp_reloc_image_apply(const uint8_t *elf,size_t len,PspCpu *cpu,
                          const PspRelocSegment *segments,size_t n,
                          size_t *out_streams) {
 if(out_streams)*out_streams=0;
 if(!elf || !cpu || !cpu->ram || !segments || !n || n>MAX_HEADERS ||
    !cpu->ram_size || cpu->ram_size>MAX_RAM_SNAPSHOT || len<52)
  return -1;
 if(memcmp(elf,"\177ELF",4) || elf[4]!=1 || elf[5]!=1 || elf[6]!=1 ||
    rd16(elf+18)!=8 || rd16(elf+40)<52)
  return -2;
 uint32_t phoff=rd32(elf+28);
 uint16_t entsize=rd16(elf+42),phnum=rd16(elf+44);
 if(entsize<32 || phnum!=n || phnum>MAX_HEADERS ||
    (uint64_t)phoff+(uint64_t)entsize*phnum>len)
  return -2;
 size_t streams=0;
 for(size_t i=0;i<n;i++) {
  const uint8_t *ph=elf+phoff+i*entsize;
  uint32_t type=rd32(ph),off=rd32(ph+4),filesz=rd32(ph+16);
  if(type==PT_LOAD) {
   uint32_t original=rd32(ph+8),memsz=rd32(ph+20);
   uint64_t mapped=segments[i].mapped_vaddr;
   if(segments[i].original_vaddr!=original ||
      segments[i].mem_size!=memsz || !memsz ||
      mapped<cpu->ram_base ||
      mapped-cpu->ram_base>cpu->ram_size ||
      memsz>cpu->ram_size-(size_t)(mapped-cpu->ram_base))
    return -2;
  }
  if(type!=PT_SCE_PSPREL && type!=PT_SCE_PSPREL2)continue;
  if(++streams>MAX_STREAMS || !filesz || (uint64_t)off+filesz>len ||
     (type==PT_SCE_PSPREL && (filesz%8 || filesz>131072u*8u)) ||
     (type==PT_SCE_PSPREL2 && filesz>524288u))
   return -2;
 }
 if(!streams)return -2;
 /* Each decoder commits only after validating all records. A single PSP
  * relocation stream therefore needs no full-RAM rollback buffer. Large
  * ET_SCE_PRX modules otherwise pay for a second 6+ MiB snapshot here.
  * Keep the snapshot below for multi-stream whole-image rollback. */
 if(streams==1u){
  for(size_t i=0;i<n;i++){
   const uint8_t *ph=elf+phoff+i*entsize;
   const uint32_t type=rd32(ph);
   if(type!=PT_SCE_PSPREL && type!=PT_SCE_PSPREL2)continue;
   const size_t off=rd32(ph+4),size=rd32(ph+16);
   const int result=type==PT_SCE_PSPREL?
     psp_reloc_apply(cpu,elf+off,size,segments,n):
     psp_reloc2_apply(cpu,elf+off,size,segments,n,(unsigned)n);
   if(result!=0)return -4;
   if(out_streams)*out_streams=1u;
   return 0;
  }
  return -2;
 }
 /* Individual decoders are atomic, but later sections may fail after an
  * earlier one succeeded. Preserve the complete original RAM image. */
 uint8_t *snapshot=(uint8_t *)malloc(cpu->ram_size);
 if(!snapshot)return -3;
 memcpy(snapshot,cpu->ram,cpu->ram_size);
 int status=0;
 for(size_t i=0;i<n;i++) {
  const uint8_t *ph=elf+phoff+i*entsize;
  uint32_t type=rd32(ph);
  if(type!=PT_SCE_PSPREL && type!=PT_SCE_PSPREL2)continue;
  size_t off=rd32(ph+4),size=rd32(ph+16);
  if(type==PT_SCE_PSPREL)
   status=psp_reloc_apply(cpu,elf+off,size,segments,n);
  else
   status=psp_reloc2_apply(cpu,elf+off,size,segments,n,(unsigned)n);
  if(status!=0)break;
 }
 if(status)memcpy(cpu->ram,snapshot,cpu->ram_size);
 free(snapshot);
 if(status)return -4;
 if(out_streams)*out_streams=streams;
 return 0;
}
