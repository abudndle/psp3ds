#include "psp_reloc.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
static uint32_t read32(const uint8_t *p) {
 return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;
}
static void write32(uint8_t *p,uint32_t v) {
 for(unsigned i=0;i<4;i++)p[i]=(uint8_t)(v>>(8*i));
}
typedef struct { size_t offset; uint32_t value; } Write;
int psp_reloc_apply(PspCpu *cpu, const uint8_t *records, size_t record_len,
                    const PspRelocSegment *segments,size_t n) {
 if (!cpu || !cpu->ram || (!records && record_len) || !segments || n==0 || n>256 ||
     record_len%8 || record_len/8>65536) return -1;
 size_t count=record_len/8;
 if(count && count>SIZE_MAX/sizeof(Write))return -1;
 Write *writes=count?(Write *)calloc(count,sizeof(Write)):NULL;
 if(count&&!writes)return -1;
 int status=-2;size_t used=0;
 for(size_t i=0;i<count;i++) {
  uint32_t rel_off=read32(records+i*8), info=read32(records+i*8+4);
  unsigned type=info&255, ofs_base=(info>>8)&255, addr_base=(info>>16)&255;
  if ((info>>24)!=0 || ofs_base>=n || addr_base>=n ||
      (type!=0 && type!=2 && type!=4)) goto cleanup;
  if(type==0)continue;
  const PspRelocSegment *target=&segments[ofs_base], *source=&segments[addr_base];
  if ((rel_off&3) || rel_off>target->mem_size || target->mem_size-rel_off<4)goto cleanup;
  uint64_t guest_addr=(uint64_t)target->mapped_vaddr+rel_off;
  if (guest_addr>UINT32_MAX || guest_addr<cpu->ram_base)goto cleanup;
  uint64_t off=guest_addr-cpu->ram_base;
  if(off>cpu->ram_size || cpu->ram_size-(size_t)off<4)goto cleanup;
  /* Avoid ambiguous double-patched words. */
  for(size_t j=0;j<used;j++)if(writes[j].offset==(size_t)off)goto cleanup;
  uint32_t old=read32(cpu->ram+(size_t)off);
  uint32_t delta=source->mapped_vaddr-source->original_vaddr;
  uint32_t value;
  if(type==2) value=old+delta;
  else {
   /* J/JAL encodes the 28-bit word-aligned target, with a PC high nibble. */
   if ((delta&3) || (old>>26!=2 && old>>26!=3))goto cleanup;
   uint32_t old_addr=(old&0x03ffffffu)<<2;
   uint32_t new_addr=old_addr+delta;
   /* High four bits of the jump target must match (PC+4) after relocation. */
   if (((new_addr^((uint32_t)guest_addr+4))&0xf0000000u)!=0)goto cleanup;
   value=(old&0xfc000000u)|((new_addr>>2)&0x03ffffffu);
  }
  writes[used].offset=(size_t)off;writes[used].value=value;used++;
 }
 for(size_t i=0;i<used;i++)write32(cpu->ram+writes[i].offset,writes[i].value);
 status=0;
cleanup:
 free(writes);
 return status;
}
