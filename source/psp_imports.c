#include "psp_imports.h"
#include <string.h>
#define JR_RA 0x03e00008u
#define SYSCALL_BASE 0x0000000cu
static void wr32(uint8_t *p, uint32_t v) {
 for (unsigned i=0;i<4;i++) p[i]=(uint8_t)(v>>(8*i));
}
static int valid_service(PspHleService s) {
 return s==PSP_HLE_SYSTEM_TIME_LOW || s==PSP_HLE_SYSTEM_TIME_WIDE ||
        s==PSP_HLE_SYSTEM_TIME ||
        s==PSP_HLE_IO_OPEN || s==PSP_HLE_IO_READ ||
        s==PSP_HLE_IO_LSEEK32 || s==PSP_HLE_IO_CLOSE;
}
int psp_imports_link(PspCpu *cpu, PspHle *hle, const PspImport *imports,
                     size_t count, const PspExport *exports, size_t export_count) {
 if (!cpu || !hle || !cpu->ram || (count && !imports) ||
     (export_count && !exports) || count>PSP_HLE_MAX_BINDINGS ||
     hle->count>PSP_HLE_MAX_BINDINGS ||
     count>PSP_HLE_MAX_BINDINGS-hle->count) return -1;
 uint32_t ids[PSP_HLE_MAX_BINDINGS];
 size_t offsets[PSP_HLE_MAX_BINDINGS];
 PspHleService services[PSP_HLE_MAX_BINDINGS];
 /* Validate the entire import batch before changing guest memory or bindings. */
 for (size_t i=0;i<count;i++) {
  const PspImport *in=&imports[i];
  if (!in->library || !*in->library || (in->stub_address&3u) ||
      in->stub_address<cpu->ram_base) return -1;
  uint64_t offset=(uint64_t)in->stub_address-cpu->ram_base;
  if (offset>cpu->ram_size || cpu->ram_size-(size_t)offset<8) return -1;
  offsets[i]=(size_t)offset;
  for (size_t j=0;j<i;j++) {
   if (offset<offsets[j]+8 && offset+8>offsets[j]) return -2;
  }
  int found=0;
  for (size_t e=0;e<export_count;e++) {
   if (exports[e].library && strcmp(in->library,exports[e].library)==0 &&
       in->nid==exports[e].nid && valid_service(exports[e].service)) {
    if (found) return -2;
    services[i]=exports[e].service; found=1;
   }
  }
  if (!found) return -3; /* Unimplemented imports must not be silently stubbed. */
  uint32_t id=0x100u;
  for (;;) {
   if (id>0xfffffu) return -1;
   int used=0;
   for (size_t b=0;b<hle->count;b++)
    if (hle->entries[b].syscall_id==id) {used=1;break;}
   for (size_t b=0;b<i;b++) if (ids[b]==id) {used=1;break;}
   if (!used) break;
   id++;
  }
  ids[i]=id;
 }
 for (size_t i=0;i<count;i++) {
  if (psp_hle_bind(hle,ids[i],services[i])!=0) return -4;
  wr32(cpu->ram+offsets[i],JR_RA);
  wr32(cpu->ram+offsets[i]+4,(ids[i]<<6)|SYSCALL_BASE);
 }
 return 0;
}
