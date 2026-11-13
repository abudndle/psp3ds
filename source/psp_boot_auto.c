#include "psp_boot.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define AUTO_MAX_RAM (32u*1024u*1024u)
#define AUTO_MAX_HEADERS 128u
#define PT_LOAD 1u

static uint16_t get16(const uint8_t *p) {
 return (uint16_t)(p[0] | (uint16_t)p[1]<<8);
}
static uint32_t get32(const uint8_t *p) {
 return (uint32_t)p[0] | (uint32_t)p[1]<<8 |
        (uint32_t)p[2]<<16 | (uint32_t)p[3]<<24;
}
static int in_ram(const PspCpu *cpu,uint64_t address,uint32_t bytes) {
 uint64_t lo=cpu->ram_base,hi=lo+cpu->ram_size;
 return address>=lo && address<=hi && bytes<=hi-address &&
        address<=UINT32_MAX && (address & 3u)==0;
}
static int overlaps(const PspRelocSegment *mapped,size_t n,
                    uint64_t addr,uint32_t size) {
 for(size_t j=0;j<n;j++) {
  if(!mapped[j].mem_size)continue;
  uint64_t prev=mapped[j].mapped_vaddr;
  if(addr<prev+mapped[j].mem_size && prev<addr+size)return 1;
 }
 return 0;
}
static int alias(const uint8_t *file,size_t length,const uint8_t *ram,size_t size) {
 /* Integer range arithmetic avoids undefined comparisons between unrelated
  * pointers. Conservatively reject address arithmetic wrapping. */
 uintptr_t a=(uintptr_t)file,b=(uintptr_t)ram;
 if(length>UINTPTR_MAX-a || size>UINTPTR_MAX-b)return 1;
 return a<b+size && b<a+length;
}
int psp_boot_auto_mapped_elf(const uint8_t *elf,size_t length,
                             PspCpu *cpu,PspHle *hle,
                             PspModuleInfo *module,size_t *streams) {
 if(module)memset(module,0,sizeof(*module));
 if(streams)*streams=0;
 if(!elf || !cpu || !hle || !module || !cpu->ram ||
    !cpu->ram_size || cpu->ram_size>AUTO_MAX_RAM ||
    length<52 || alias(elf,length,cpu->ram,cpu->ram_size))
  return PSP_BOOT_BAD_ARGUMENT;
 if(memcmp(elf,"\177ELF",4) || elf[4]!=1 || elf[5]!=1 || elf[6]!=1 ||
    (get16(elf+16)!=2 && get16(elf+16)!=0xffa0u) || get16(elf+18)!=8 || get16(elf+40)<52)
  return PSP_BOOT_ELF_FAILED;
 uint32_t phoff=get32(elf+28),entry=get32(elf+24);
 int prx = get16(elf+16) == 0xffa0u;
 if (prx && entry > UINT32_MAX - cpu->ram_base)
  return PSP_BOOT_MAPPING_FAILED;
 if (prx) entry += cpu->ram_base;
 size_t entsize=get16(elf+42),count=get16(elf+44);
 if(!count || count>AUTO_MAX_HEADERS || entsize<32 ||
    (uint64_t)phoff+(uint64_t)entsize*count>length || (entry & 3u))
  return PSP_BOOT_MAPPING_FAILED;
 PspRelocSegment mapped[AUTO_MAX_HEADERS];
 memset(mapped,0,sizeof(mapped));
 size_t first=SIZE_MAX;
 uint64_t high_water=cpu->ram_base;
 int entry_valid=0;
 for(size_t i=0;i<count;i++) {
  const uint8_t *ph=elf+phoff+i*entsize;
  if(get32(ph)!=PT_LOAD)continue;
  uint32_t offset=get32(ph+4),original=get32(ph+8);
  uint32_t preferred=get32(ph+12),filesz=get32(ph+16);
  uint32_t memsz=get32(ph+20),flags=get32(ph+24);
  if(!memsz || memsz<filesz || (uint64_t)offset+filesz>length)
   return PSP_BOOT_MAPPING_FAILED;
  uint64_t addr=original;
  if (prx) {
   if (!in_ram(cpu, (uint64_t)cpu->ram_base + original, memsz))
    return PSP_BOOT_MAPPING_FAILED;
   addr = (uint64_t)cpu->ram_base + original;
   if (first == SIZE_MAX) first = i;
  } else if(first==SIZE_MAX) {
   first=i;
   /* No entry rebasing or section-table fixups in this first increment. */
   if(!in_ram(cpu,addr,memsz) ||
      (preferred && preferred!=original))
    return PSP_BOOT_MAPPING_FAILED;
  } else if(preferred) {
   addr=preferred;
   if(!in_ram(cpu,addr,memsz) || overlaps(mapped,i,addr,memsz))
    return PSP_BOOT_MAPPING_FAILED;
  } else if(!in_ram(cpu,addr,memsz) || overlaps(mapped,i,addr,memsz)) {
   /* Deterministic first-fit after prior mapped segments; only when no
    * explicit address hint was supplied by the ELF. */
   addr=(high_water+15u)&~UINT64_C(15);
   if(!in_ram(cpu,addr,memsz) || overlaps(mapped,i,addr,memsz))
    return PSP_BOOT_MAPPING_FAILED;
  }
  if(overlaps(mapped,i,addr,memsz))return PSP_BOOT_MAPPING_FAILED;
  mapped[i].original_vaddr=original;
  mapped[i].mapped_vaddr=(uint32_t)addr;
  mapped[i].mem_size=memsz;
  if(addr+memsz>high_water)high_water=addr+memsz;
  if((flags&1u) && entry>=addr && (uint64_t)entry+4u<=addr+filesz)
   entry_valid=1;
 }
 if(first==SIZE_MAX || !entry_valid)return PSP_BOOT_MAPPING_FAILED;
 uint8_t *snapshot=(uint8_t *)malloc(cpu->ram_size);
 if(!snapshot)return PSP_BOOT_NO_MEMORY;
 memcpy(snapshot,cpu->ram,cpu->ram_size);
 for(size_t i=0;i<count;i++) {
  if(!mapped[i].mem_size)continue;
  const uint8_t *ph=elf+phoff+i*entsize;
  size_t off=(size_t)(mapped[i].mapped_vaddr-cpu->ram_base);
  size_t size=get32(ph+16);
  memset(cpu->ram+off,0,mapped[i].mem_size);
  memcpy(cpu->ram+off,elf+get32(ph+4),size);
 }
 /* Existing boot stage handles relocation and module/HLE linkage, and
  * itself rolls back modifications after a failure. The outer snapshot
  * also rolls back the initial PT_LOAD copies and BSS zero-fill. */
 PspModuleInfo staged_module;
 size_t staged_streams=0;
 int result=psp_boot_mapped_relocated_elf(elf,length,cpu,hle,
                 mapped,count,&staged_module,&staged_streams);
 if(result!=PSP_BOOT_OK)
  memcpy(cpu->ram,snapshot,cpu->ram_size);
 free(snapshot);
 if(result!=PSP_BOOT_OK)return result;
 *module=staged_module;
 if(streams)*streams=staged_streams;
 return PSP_BOOT_OK;
}
