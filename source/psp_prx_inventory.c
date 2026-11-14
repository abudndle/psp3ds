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
