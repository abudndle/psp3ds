#include "psp_prx.h"
#include <string.h>
static uint16_t rd16(const uint8_t *p) {
 return (uint16_t)((uint16_t)p[0] | ((uint16_t)p[1]<<8));
}
static uint32_t rd32(const uint8_t *p) {
 return (uint32_t)p[0] | (uint32_t)p[1]<<8 |
        (uint32_t)p[2]<<16 | (uint32_t)p[3]<<24;
}
static const uint8_t *guest(const PspCpu *cpu,uint32_t addr,size_t n) {
 if (!cpu || !cpu->ram || addr < cpu->ram_base) return NULL;
 uint64_t offset=(uint64_t)addr-cpu->ram_base;
 if (offset > cpu->ram_size || n > cpu->ram_size-(size_t)offset) return NULL;
 return cpu->ram+(size_t)offset;
}
int psp_prx_parse_imports(const PspCpu *cpu,uint32_t start,
                          uint32_t end,PspPrxImports *out) {
 if (!out) return -1;
 memset(out,0,sizeof(*out));
 if (!cpu || end<start || (start&3u) || (end&3u) ||
     !guest(cpu,start,(size_t)((uint64_t)end-start))) return -1;
 uint32_t cursor=start;
 while (cursor<end) {
  const uint8_t *h=guest(cpu,cursor,20);
  if (!h || (uint64_t)end-cursor<20) goto malformed;
  const uint32_t name=rd32(h);
  const unsigned words=h[8], vars=h[9], funcs=rd16(h+10);
  if ((words!=5 && words!=6) || vars!=0 || funcs==0 ||
      (uint64_t)words*4u>(uint64_t)end-cursor) goto malformed;
  uint32_t nids=rd32(h+12), stubs=rd32(h+16);
  if (!guest(cpu,nids,(size_t)funcs*4u) ||
      !guest(cpu,stubs,(size_t)funcs*8u)) goto malformed;
  char tmp[PSP_PRX_MAX_NAME];
  size_t k;
  for (k=0;k<PSP_PRX_MAX_NAME;k++) {
   const uint8_t *c=guest(cpu,name+(uint32_t)k,1);
   if (!c) goto malformed;
   tmp[k]=(char)*c;
   if (!tmp[k]) break;
  }
  if (k==0 || k==PSP_PRX_MAX_NAME) goto malformed;
  if ((size_t)funcs>PSP_HLE_MAX_BINDINGS-out->count) goto malformed;
  for (unsigned i=0;i<funcs;i++) {
   size_t idx=out->count++;
   memcpy(out->names[idx],tmp,k+1);
   out->imports[idx].library=out->names[idx];
   out->imports[idx].nid=rd32(guest(cpu,nids+4u*i,4));
   out->imports[idx].stub_address=stubs+8u*i;
  }
  cursor+=(uint32_t)words*4u;
 }
 return 0;
malformed:
 memset(out,0,sizeof(*out));
 return -2;
}
