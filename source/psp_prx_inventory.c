#include "psp_prx_inventory.h"
#include <stdint.h>
#include <string.h>

static uint16_t u16(const uint8_t *p){
 return (uint16_t)(p[0] | (uint16_t)p[1]<<8);
}
static uint32_t u32(const uint8_t *p){
 return (uint32_t)p[0] | (uint32_t)p[1]<<8 |
        (uint32_t)p[2]<<16 | (uint32_t)p[3]<<24;
}
static const uint8_t *mapped(const PspCpu *cpu,uint64_t addr,size_t len){
 if(!cpu || !cpu->ram || addr<cpu->ram_base ||
    addr>UINT32_MAX)return NULL;
 const uint64_t offset=addr-cpu->ram_base;
 if(offset>cpu->ram_size || len>cpu->ram_size-(size_t)offset)
  return NULL;
 return cpu->ram+(size_t)offset;
}
int psp_prx_import_inventory(const PspCpu *cpu,uint32_t top,uint32_t end,
                             PspPrxImportInventory *out){
 if(out)memset(out,0,sizeof(*out));
 if(!out || !cpu || !cpu->ram || end<top ||
    (top&3u) || (end&3u) || !mapped(cpu,top,(size_t)((uint64_t)end-top)))
  return -1;
 PspPrxImportInventory staged;
 memset(&staged,0,sizeof(staged));
 uint64_t cursor=top;
 while(cursor<end){
  if(staged.library_count>=PSP_PRX_INVENTORY_MAX_LIBRARIES ||
     (uint64_t)end-cursor<20u)return -2;
  const uint8_t *h=mapped(cpu,cursor,20u);
  if(!h)return -2;
  const uint32_t name=u32(h);
  const unsigned words=h[8],variables=h[9],functions=u16(h+10);
  const uint32_t nids=u32(h+12),stubs=u32(h+16);
  if((words!=5u && words!=6u) || variables!=0 || !functions ||
     (uint64_t)words*4u>(uint64_t)end-cursor ||
     functions>PSP_PRX_INVENTORY_MAX_FUNCTIONS-staged.function_count ||
     !mapped(cpu,nids,(size_t)functions*4u) ||
     !mapped(cpu,stubs,(size_t)functions*8u))return -2;
  PspPrxLibraryInventory *lib=&staged.libraries[staged.library_count];
  unsigned k;
  for(k=0;k<PSP_PRX_MAX_NAME;k++){
   const uint8_t *ch=mapped(cpu,(uint64_t)name+k,1u);
   if(!ch)return -2;
   const unsigned v=*ch;
   if(!v)break;
   /* Restrict library names to printable ASCII, so an untrusted file
    * cannot inject terminal escape/control sequences into diagnostics. */
   if(v<0x21u||v>0x7eu)return -2;
   lib->name[k]=(char)v;
  }
  if(k==0u||k==PSP_PRX_MAX_NAME)return -2;
  lib->nid_table=nids;
  lib->stub_table=stubs;
  lib->first_nid=u32(mapped(cpu,nids,4u));
  lib->function_count=(uint16_t)functions;
  staged.library_count++;
  staged.function_count+=(uint16_t)functions;
  cursor+=(uint64_t)words*4u;
 }
 *out=staged;
 return 0;
}

int psp_prx_import_coverage(const PspCpu *cpu,
                            const PspPrxImportInventory *inventory,
                            PspPrxHleCoverage *out){
 if(out)memset(out,0,sizeof(*out));
 if(!cpu||!cpu->ram||!inventory||!out||
    inventory->library_count>PSP_PRX_INVENTORY_MAX_LIBRARIES ||
    inventory->function_count>PSP_PRX_INVENTORY_MAX_FUNCTIONS)return -1;
 unsigned total=0,implemented=0;
 for(unsigned i=0;i<inventory->library_count;i++){
  const PspPrxLibraryInventory *lib=&inventory->libraries[i];
  const size_t count=lib->function_count;
  if(!count || count>PSP_PRX_INVENTORY_MAX_FUNCTIONS-total)return -2;
  const char *name=lib->name;
  unsigned n=0;
  for(;n<PSP_PRX_MAX_NAME;n++){
   unsigned char character=(unsigned char)name[n];
   if(!character)break;
   if(character<0x21u||character>0x7eu)return -2;
  }
  if(n==0 || n>=PSP_PRX_MAX_NAME)return -2;
  const uint8_t *nids=mapped(cpu,lib->nid_table,count*4u);
  if(!nids)return -2;
  for(size_t j=0;j<count;j++){
   PspHleService service;
   const int known=psp_imports_known_service(name,u32(nids+j*4u),
                                             &service);
   if(known<0)return -2;
   if(known==1)implemented++;
  }
  total+=(unsigned)count;
 }
 if(total!=inventory->function_count)return -2;
 out->implemented=(uint16_t)implemented;
 out->unsupported=(uint16_t)(total-implemented);
 return 0;
}

int psp_prx_link_verified_imports(PspCpu *cpu,PspHle *hle,
                                  const PspPrxImportInventory *inv,
                                  PspPrxHleCoverage *out){
 if(out)memset(out,0,sizeof(*out));
 if(!cpu||!cpu->ram||!hle||!inv||!out)return -1;
 PspPrxHleCoverage coverage;
 if(psp_prx_import_coverage(cpu,inv,&coverage)!=0 ||
    coverage.implemented>PSP_HLE_MAX_BINDINGS ||
    coverage.implemented>PSP_HLE_MAX_BINDINGS-hle->count)
  return -2;
 PspImport selected[PSP_HLE_MAX_BINDINGS];
 unsigned total=0,found=0;
 /* Preflight ALL library stub ranges and NID addresses, even when their
  * firmware functions are unsupported. The existing import linker then
  * validates every selected stub and commits the group atomically. */
 for(unsigned i=0;i<inv->library_count;i++){
  const PspPrxLibraryInventory *lib=&inv->libraries[i];
  const uint32_t count=lib->function_count;
  const uint8_t *nids=mapped(cpu,lib->nid_table,(size_t)count*4u);
  if(!nids || !mapped(cpu,lib->stub_table,(size_t)count*8u))return -2;
  for(unsigned j=0;j<count;j++){
   const uint32_t nid=u32(nids+(size_t)j*4u);
   PspHleService service;
   const int supported=psp_imports_known_service(lib->name,nid,&service);
   if(supported<0)return -2;
   if(supported){
    if(found>=PSP_HLE_MAX_BINDINGS)return -2;
    const uint64_t addr=(uint64_t)lib->stub_table+8u*j;
    if(addr>UINT32_MAX)return -2;
    selected[found].library=lib->name;
    selected[found].nid=nid;
    selected[found].stub_address=(uint32_t)addr;
    found++;
   }
  }
  total+=count;
 }
 if(total!=inv->function_count || found!=coverage.implemented)return -2;
 if(found && psp_imports_link_known(cpu,hle,selected,found)!=0)return -3;
 *out=coverage;
 return 0;
}
