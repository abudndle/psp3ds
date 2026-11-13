#include "psp_module.h"
#include <string.h>
#include <stdint.h>
static uint16_t u16(const uint8_t *p) { return (uint16_t)(p[0] | ((uint16_t)p[1]<<8)); }
static uint32_t u32(const uint8_t *p) {
 return (uint32_t)p[0] | (uint32_t)p[1]<<8 | (uint32_t)p[2]<<16 | (uint32_t)p[3]<<24;
}
static const uint8_t *guest(const PspCpu *cpu, uint32_t addr, size_t len) {
 if (!cpu || !cpu->ram || addr < cpu->ram_base) return NULL;
 uint64_t off=(uint64_t)addr-cpu->ram_base;
 if (off>cpu->ram_size || len>cpu->ram_size-(size_t)off) return NULL;
 return cpu->ram+(size_t)off;
}
static int range(size_t n,uint64_t off,uint64_t bytes){return off<=n && bytes<=n-off;}
int psp_module_discover(const uint8_t *elf,size_t size,
                        const PspCpu *cpu,PspModuleInfo *out) {
 if (!out) return -1;
 memset(out,0,sizeof(*out));
 if (!elf || !cpu || size<52 || memcmp(elf,"\177ELF",4)!=0 ||
     elf[4]!=1 || elf[5]!=1 || elf[6]!=1 || u16(elf+18)!=8) return -1;
 uint32_t shoff=u32(elf+32);
 uint16_t entsize=u16(elf+46), count=u16(elf+48), names=u16(elf+50);
 if (entsize<40 || count==0 || count>1024 || names>=count ||
     !range(size,shoff,(uint64_t)count*entsize)) return -2;
 const uint8_t *strhdr=elf+shoff+(size_t)names*entsize;
 uint32_t stroff=u32(strhdr+16), strlen=u32(strhdr+20);
 if (!range(size,stroff,strlen) || strlen==0) return -2;
 const uint8_t *strtab=elf+stroff;
 int found=0;
 uint32_t address=0;
 for (unsigned i=0;i<count;i++) {
  const uint8_t *sh=elf+shoff+(size_t)i*entsize;
  uint32_t noff=u32(sh), a=u32(sh+12), sectlen=u32(sh+20);
  if (noff>=strlen) return -2;
  const uint8_t *name=strtab+noff;
  const uint8_t *end=memchr(name,0,strlen-noff);
  if (!end) return -2;
  if ((size_t)(end-name)==21 && memcmp(name,".rodata.sceModuleInfo",21)==0) {
   /* The PSP SceModuleInfo structure occupies 52 bytes. */
   if (found || sectlen<52 || !guest(cpu,a,52)) return -2;
   found=1;address=a;
  }
 }
 if (!found) return -3;
 const uint8_t *p=guest(cpu,address,52);
 /* 27-byte name field followed by a terminating byte at offset 31. */
 if (p[4]==0 || p[31]!=0 || !memchr(p+4,0,28)) return -2;
 out->attributes=u16(p);out->version_minor=p[2];out->version_major=p[3];
 memcpy(out->name,p+4,28);
 out->gp=u32(p+32);out->export_top=u32(p+36);out->export_end=u32(p+40);
 out->import_top=u32(p+44);out->import_end=u32(p+48);
 out->module_address=address;
 if (out->import_end<out->import_top ||
     !guest(cpu,out->import_top,(uint64_t)out->import_end-out->import_top) ||
     out->export_end<out->export_top ||
     !guest(cpu,out->export_top,(uint64_t)out->export_end-out->export_top)) {
  memset(out,0,sizeof(*out));return -2;
 }
 return 0;
}
int psp_module_imports(const PspCpu *cpu,const PspModuleInfo *module,
                       PspPrxImports *out) {
 if (!module || !out) return -1;
 return psp_prx_parse_imports(cpu,module->import_top,module->import_end,out);
}

int psp_module_link_known(const uint8_t *elf, size_t elf_size,
                          PspCpu *cpu, PspHle *hle, PspModuleInfo *out) {
 if (out) memset(out,0,sizeof(*out));
 if (!cpu || !hle) return -1;
 PspModuleInfo info;
 int status=psp_module_discover(elf,elf_size,cpu,&info);
 if (status) return status;
 PspPrxImports parsed;
 status=psp_module_imports(cpu,&info,&parsed);
 if (status) return status;
 if (parsed.count && psp_imports_link_known(cpu,hle,parsed.imports,parsed.count)!=0)
  return -4;
 if (out) *out=info;
 return 0;
}
