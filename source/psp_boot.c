#include "psp_boot.h"
#include <string.h>

int psp_boot_plain_elf(const uint8_t *elf, size_t elf_size,
                       PspCpu *cpu, PspHle *hle,
                       PspLoadResult *loaded, PspModuleInfo *module) {
 if (loaded) memset(loaded, 0, sizeof(*loaded));
 if (module) memset(module, 0, sizeof(*module));
 if (!elf || !cpu || !hle || !cpu->ram || !loaded || !module ||
     !cpu->ram_size) return PSP_BOOT_BAD_ARGUMENT;
 PspLoadResult staged;
 if (psp_elf_load(elf, elf_size, cpu->ram, cpu->ram_size,
                  cpu->ram_base, &staged) != PSP_LOAD_OK)
  return PSP_BOOT_ELF_FAILED;
 /* Keep CPU registers unchanged until all imports have been validated. */
 PspCpu candidate = *cpu;
 psp_cpu_init(&candidate, cpu->ram, cpu->ram_size, cpu->ram_base,
              staged.entry);
 PspModuleInfo info;
 if (psp_module_link_known(elf, elf_size, &candidate, hle, &info) != 0)
  return PSP_BOOT_IMPORT_FAILED;
 *cpu = candidate;
 *loaded = staged;
 *module = info;
 return PSP_BOOT_OK;
}

/* Compose relocation and import linking as one transaction. Existing HLE
 * syscall allocations are staged in a copy, and guest RAM is restored on
 * either relocation or import discovery failure. */
#include <stdlib.h>
#include <stdint.h>
#define BOOT_MAX_RAM (32u*1024u*1024u)
static uint16_t boot_u16(const uint8_t *p) {
 return (uint16_t)(p[0]|(uint16_t)p[1]<<8);
}
static uint32_t boot_u32(const uint8_t *p) {
 return (uint32_t)p[0]|(uint32_t)p[1]<<8|
        (uint32_t)p[2]<<16|(uint32_t)p[3]<<24;
}
/* Compare distances to avoid overflow at either range endpoint. */
static int boot_ranges_overlap(const void *first, size_t first_size,
                               const void *second, size_t second_size) {
 uintptr_t a = (uintptr_t)first;
 uintptr_t b = (uintptr_t)second;
 return a <= b ? b - a < first_size : a - b < second_size;
}

int psp_boot_mapped_relocated_elf(const uint8_t *elf, size_t elf_size,
                                  PspCpu *cpu, PspHle *hle,
                                  const PspRelocSegment *segments,
                                  size_t count, PspModuleInfo *module,
                                  size_t *relocation_streams) {
 if(module) memset(module,0,sizeof(*module));
 if(relocation_streams) *relocation_streams=0;
 if(!elf || !cpu || !hle || !module || !segments || !cpu->ram ||
    cpu->ram_size==0 || cpu->ram_size>BOOT_MAX_RAM ||
    count==0 || count>128 || elf_size<52 ||
    boot_ranges_overlap(elf, elf_size, cpu->ram, cpu->ram_size))
  return PSP_BOOT_BAD_ARGUMENT;
 /* Restrict to existing fixed-address ELF ET_EXEC subset; PRX ET_REL and
  * unsupported image formats need dedicated mapping and entry handling. */
 if(memcmp(elf,"\177ELF",4) || elf[4]!=1 || elf[5]!=1 ||
    elf[6]!=1 || boot_u16(elf+16)!=2 || boot_u16(elf+18)!=8)
  return PSP_BOOT_ELF_FAILED;
 uint32_t entry=boot_u32(elf+24);
 uint32_t phoff=boot_u32(elf+28);
 uint16_t entsize=boot_u16(elf+42),phnum=boot_u16(elf+44);
 if((entry&3u) || entsize<32 || phnum!=count ||
    (uint64_t)phoff+(uint64_t)entsize*count>elf_size)
  return PSP_BOOT_ELF_FAILED;
 int executable=0;
 for(size_t i=0;i<count;i++){
  const uint8_t *ph=elf+phoff+i*entsize;
  if(boot_u32(ph)!=1u)continue;
  uint32_t original=boot_u32(ph+8),filesz=boot_u32(ph+16);
  uint32_t memsz=boot_u32(ph+20),flags=boot_u32(ph+24);
  uint64_t mapped=segments[i].mapped_vaddr;
  if(segments[i].original_vaddr!=original || segments[i].mem_size!=memsz ||
     filesz>memsz || mapped<cpu->ram_base ||
     mapped-cpu->ram_base>cpu->ram_size ||
     memsz>cpu->ram_size-(size_t)(mapped-cpu->ram_base))
   return PSP_BOOT_ELF_FAILED;
  if((flags&1u) && entry>=mapped &&
     (uint64_t)entry+4u<=mapped+filesz) executable=1;
 }
 if(!executable)return PSP_BOOT_ELF_FAILED;
 uint8_t *snapshot=(uint8_t*)malloc(cpu->ram_size);
 if(!snapshot)return PSP_BOOT_NO_MEMORY;
 memcpy(snapshot,cpu->ram,cpu->ram_size);
 PspCpu candidate=*cpu;
 PspHle staged_hle=*hle;
 size_t streams=0;
 int status=PSP_BOOT_RELOCATION_FAILED;
 if(psp_reloc_image_apply(elf,elf_size,&candidate,segments,count,&streams))
  goto rollback;
 PspModuleInfo staged_module;
 if(psp_module_link_known(elf,elf_size,&candidate,&staged_hle,
                          &staged_module)){
  status=PSP_BOOT_IMPORT_FAILED;
  goto rollback;
 }
 psp_cpu_init(&candidate,cpu->ram,cpu->ram_size,cpu->ram_base,entry);
 *cpu=candidate;
 *hle=staged_hle;
 *module=staged_module;
 if(relocation_streams)*relocation_streams=streams;
 free(snapshot);
 return PSP_BOOT_OK;
rollback:
 memcpy(cpu->ram,snapshot,cpu->ram_size);
 free(snapshot);
 return status;
}
