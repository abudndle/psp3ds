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
typedef struct { size_t offset; unsigned addr_base; uint32_t old; } PendingHi;
int psp_reloc_apply(PspCpu *cpu, const uint8_t *records, size_t record_len,
                    const PspRelocSegment *segments,size_t n) {
 if (!cpu || !cpu->ram || (!records && record_len) || !segments || n==0 || n>256 ||
     record_len%8 || record_len/8>PSP_RELOC_MAX_RECORDS) return -1;
 size_t count=record_len/8;
 if(count && (count>SIZE_MAX/sizeof(Write) || count>SIZE_MAX/sizeof(PendingHi)))return -1;
 Write *writes=count?(Write *)calloc(count,sizeof(Write)):NULL;
 PendingHi *pending=count?(PendingHi *)calloc(count,sizeof(PendingHi)):NULL;
 /* One bit per guest word in the largest relocation target range. */
 size_t target_end = 0;
 for (size_t i = 0; i < n; ++i) {
  if (!segments[i].mem_size || segments[i].mapped_vaddr < cpu->ram_base)
   continue;
  uint64_t end = (uint64_t)(segments[i].mapped_vaddr - cpu->ram_base) +
                 segments[i].mem_size;
  if (end <= cpu->ram_size && end > target_end) target_end = (size_t)end;
 }
 size_t bitmap_size = target_end / 32u + (target_end % 32u != 0);
 uint8_t *seen = count && bitmap_size ? calloc(bitmap_size, 1) : NULL;
 if (count && (!writes || !pending || (bitmap_size && !seen))) {
  free(writes);
  free(pending);
  free(seen);
  return -1;
 }
 int status=-2;size_t used=0,hi_count=0;
 for(size_t i=0;i<count;i++) {
  uint32_t rel_off=read32(records+i*8), info=read32(records+i*8+4);
  unsigned type=info&255, ofs_base=(info>>8)&255, addr_base=(info>>16)&255;
  if ((info>>24)!=0 || ofs_base>=n || addr_base>=n ||
      (type!=0 && type!=2 && type!=4 && type!=5 && type!=6)) goto cleanup;
  if(type==0)continue;
  const PspRelocSegment *target=&segments[ofs_base], *source=&segments[addr_base];
  if ((rel_off&3) || rel_off>target->mem_size || target->mem_size-rel_off<4)goto cleanup;
  uint64_t guest_addr=(uint64_t)target->mapped_vaddr+rel_off;
  if (guest_addr>UINT32_MAX || guest_addr<cpu->ram_base)goto cleanup;
  uint64_t off=guest_addr-cpu->ram_base;
  if(off>cpu->ram_size || cpu->ram_size-(size_t)off<4)goto cleanup;
  /* Guest relocation targets must be aligned and unique. */
  if ((off & 3u) || off >= target_end) goto cleanup;
  size_t slot = (size_t)off / 4u;
  uint8_t mask = (uint8_t)(1u << (slot % 8u));
  if (seen[slot / 8u] & mask) goto cleanup;
  seen[slot / 8u] |= mask;
  uint32_t old=read32(cpu->ram+(size_t)off);
  uint32_t delta=source->mapped_vaddr-source->original_vaddr;
  uint32_t value;
  if(type==5) {
   /* HI16 carries are determined by the sign-extended LO16 addend. */
   if((old>>26)!=15)goto cleanup; /* LUI */
   pending[hi_count].offset=(size_t)off;
   pending[hi_count].addr_base=addr_base;
   pending[hi_count].old=old;
   hi_count++;
   continue;
  }
  if(type==6) {
   /* Paired low halves must sign-extend; ORI is allowed standalone only. */
   unsigned opcode=old>>26;
   if(opcode!=9 && (opcode!=13 || hi_count))goto cleanup; /* ADDIU, or standalone ORI */
   int32_t low=(int32_t)(int16_t)(old&0xffffu);
   for(size_t j=0;j<hi_count;j++) {
    if(pending[j].addr_base!=addr_base || used>=count)goto cleanup;
    uint32_t combined=((pending[j].old&0xffffu)<<16)+(uint32_t)low+delta;
    uint32_t high=((combined+0x8000u)>>16)&0xffffu;
    writes[used].offset=pending[j].offset;
    writes[used].value=(pending[j].old&0xffff0000u)|high;
    used++;
   }
   hi_count=0;
   value=(old&0xffff0000u)|(((uint32_t)low+delta)&0xffffu);
  } else if(type==2) value=old+delta;
  else {
   /* J/JAL encodes a 28-bit target; upper nibble comes from PC+4. */
   if ((delta&3) || (old>>26!=2 && old>>26!=3))goto cleanup;
   uint32_t old_addr=(old&0x03ffffffu)<<2;
   uint32_t new_addr=old_addr+delta;
   if (((new_addr^((uint32_t)guest_addr+4))&0xf0000000u)!=0)goto cleanup;
   value=(old&0xfc000000u)|((new_addr>>2)&0x03ffffffu);
  }
  if(used>=count)goto cleanup;
  writes[used].offset=(size_t)off;writes[used].value=value;used++;
 }
 if(hi_count)goto cleanup; /* No LO16 was supplied for these HI16 values. */
 for(size_t i=0;i<used;i++)write32(cpu->ram+writes[i].offset,writes[i].value);
 status=0;
cleanup:
 free(writes);free(pending);free(seen);
 return status;
}
