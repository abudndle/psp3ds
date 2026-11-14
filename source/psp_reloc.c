#include "psp_reloc.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/* Large PSP PRX modules can have more than 65,536 relocation records.
 * A bounded 131,072 record limit is enough for the private Jetpack
 * compatibility sample while staying below one MiB of input records. */
#define PSP_RELOC_MAX_RECORDS 131072u

static uint32_t read32(const uint8_t *p) {
 return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;
}
static void write32(uint8_t *p,uint32_t v) {
 for(unsigned i=0;i<4;i++)p[i]=(uint8_t)(v>>(8*i));
}
typedef struct { size_t offset; uint32_t value; } Write;
typedef struct { size_t offset; unsigned addr_base; uint32_t old; } PendingHi;
static int compare_writes(const void *a,const void *b) {
 const Write *first=(const Write*)a,*second=(const Write*)b;
 return first->offset<second->offset?-1:
        first->offset>second->offset?1:0;
}
int psp_reloc_apply(PspCpu *cpu, const uint8_t *records, size_t record_len,
                    const PspRelocSegment *segments,size_t n) {
 if (!cpu || !cpu->ram || (!records && record_len) || !segments || n==0 || n>256 ||
     record_len%8 || record_len/8>PSP_RELOC_MAX_RECORDS) return -1;
 size_t count=record_len/8;
 if(count && (count>SIZE_MAX/sizeof(Write) || count>SIZE_MAX/sizeof(PendingHi)))return -1;
 Write *writes=count?(Write *)calloc(count,sizeof(Write)):NULL;
 PendingHi *pending=count?(PendingHi *)calloc(count,sizeof(PendingHi)):NULL;
 if(count && (!writes || !pending)){free(writes);free(pending);return -1;}
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
  /* Detect duplicate targets once after staging and sorting the write set.
   * Repeated linear duplicate scans were quadratic on real large PRXs. */
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
   /* R_MIPS_LO16 commonly relocates signed memory-access offsets,
    * including LW/SW/LWC1/SWC1. The offset is sign-extended even
    * when the instruction is not ADDIU. Standalone ORI is retained
    * for compatibility, but no ORI with pending HI16 is accepted. */
   if(opcode!=9 && opcode!=32 && opcode!=33 && opcode!=35 &&
      opcode!=36 && opcode!=37 && opcode!=40 && opcode!=41 &&
      opcode!=43 && opcode!=49 && opcode!=57 &&
      (opcode!=13 || hi_count))goto cleanup;
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
 /* Validate every target in O(N log N) rather than O(N^2).
  * Sorting staged writes is safe because no guest byte has changed.
  * Reject duplicate target writes before applying anything. */
 qsort(writes,used,sizeof(*writes),compare_writes);
 for(size_t i=1;i<used;i++)if(writes[i-1].offset==writes[i].offset)goto cleanup;
 for(size_t i=0;i<used;i++)write32(cpu->ram+writes[i].offset,writes[i].value);
 status=0;
cleanup:
 free(writes);free(pending);
 return status;
}
