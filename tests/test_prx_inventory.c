#include "psp_prx_inventory.h"
#include "psp_prx_image.h"
#include "psp_module.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define GUEST_BASE UINT32_C(0x08800000)
#define ELF_SIZE 8192u
#define RAM_SIZE 6144u
#define LIB_COUNT 19u
#define FUNC_COUNT 145u
#define ELF_LOAD_OFFSET 0x100u
static uint8_t elf[ELF_SIZE],ram[RAM_SIZE];
static void w16(uint8_t *p,unsigned v){
 p[0]=(uint8_t)v;p[1]=(uint8_t)(v>>8);
}
static void w32(uint8_t *p,uint32_t v){
 for(unsigned i=0;i<4;i++)p[i]=(uint8_t)(v>>(8u*i));
}
static uint32_t r32(const uint8_t *p){
 return (uint32_t)p[0]|(uint32_t)p[1]<<8|
        (uint32_t)p[2]<<16|(uint32_t)p[3]<<24;
}
static uint8_t *guest_elf(unsigned off){return elf+ELF_LOAD_OFFSET+off;}
static void make_fixture(void){
 memset(elf,0,sizeof elf);
 memcpy(elf,"\177ELF",4);elf[4]=1;elf[5]=1;elf[6]=1;
 w16(elf+16,0xffa0u);w16(elf+18,8);
 w32(elf+20,1u);w32(elf+24,0u);w32(elf+28,52u);
 w32(elf+32,0x1d00u);w16(elf+40,52u);
 w16(elf+42,32u);w16(elf+44,2u);
 w16(elf+46,40u);w16(elf+48,3u);w16(elf+50,1u);
 /* A single file-backed PT_LOAD, plus a PSPREL command stream. */
 w32(elf+52,1u);w32(elf+56,ELF_LOAD_OFFSET);w32(elf+60,0u);
 w32(elf+68,0x1600u);w32(elf+72,0x1700u);w32(elf+76,5u);
 w32(elf+84,0x700000a0u);w32(elf+88,0x1800u);
 w32(elf+100,(4u+LIB_COUNT*3u)*8u);
 w32(guest_elf(0),0x24080007u);
 uint8_t *module=guest_elf(0x900u);
 module[2]=1;module[3]=1;
 memcpy(module+4,"Synthetic145",13u);
 w32(module+36,0x880u);w32(module+40,0x880u);
 w32(module+44,0x400u);
 w32(module+48,0x400u+LIB_COUNT*20u);
 const char strtab[]="\0.shstrtab\0.rodata.sceModuleInfo\0";
 memcpy(elf+0x1b00u,strtab,sizeof strtab);
 uint8_t *sh1=elf+0x1d00u+40u;
 w32(sh1,1u);w32(sh1+4,3u);w32(sh1+16,0x1b00u);
 w32(sh1+20,(uint32_t)sizeof strtab);
 uint8_t *sh2=elf+0x1d00u+80u;
 w32(sh2,11u);w32(sh2+4,1u);w32(sh2+8,3u);
 w32(sh2+12,0x900u);w32(sh2+16,ELF_LOAD_OFFSET+0x900u);
 w32(sh2+20,52u);
 /* Synthetic export and import pointers are PSP-relative. */
 unsigned records=0;
 const unsigned module_ptrs[]={0x900u+36u,0x900u+40u,
                               0x900u+44u,0x900u+48u};
 for(unsigned i=0;i<4u;i++){
  w32(elf+0x1800u+records*8u,module_ptrs[i]);
  w32(elf+0x1800u+records*8u+4u,2u);
  records++;
 }
 unsigned functions=0u;
 for(unsigned i=0;i<LIB_COUNT;i++){
  const unsigned n=i==LIB_COUNT-1u?19u:7u;
  uint8_t *h=guest_elf(0x400u+i*20u);
  const uint32_t name_addr=0x700u+i*12u;
  const uint32_t nids=0xa00u+functions*4u;
  const uint32_t stubs=0xc80u+functions*8u;
  w32(h,name_addr);h[8]=5;w16(h+10,n);
  w32(h+12,nids);w32(h+16,stubs);
  char name[12];
  int written=snprintf(name,sizeof name,"MockLib%02u",i);
  assert(written>0 && written<(int)sizeof name);
  memcpy(guest_elf(name_addr),name,(size_t)written+1u);
  for(unsigned f=0;f<n;f++)
   w32(guest_elf(nids+f*4u),0x10000000u+functions+f);
  functions+=n;
  const unsigned pointer_offsets[]={0u,12u,16u};
  for(unsigned j=0;j<3u;j++){
   w32(elf+0x1800u+records*8u,0x400u+i*20u+pointer_offsets[j]);
   w32(elf+0x1800u+records*8u+4u,2u);
   records++;
  }
 }
 assert(functions==FUNC_COUNT && records==4u+LIB_COUNT*3u);
}
int main(void){
 make_fixture();
 memset(ram,0xb1,sizeof ram);
 PspCpu cpu;
 psp_cpu_init(&cpu,ram,sizeof ram,GUEST_BASE,GUEST_BASE+8u);
 PspPrxImageResult image;
 assert(psp_prx_image_map_relocated(elf,sizeof elf,&cpu,&image)==0);
 assert(image.entry==GUEST_BASE&&image.load_segments==1u);
 assert(image.relocation_streams==1u&&image.image_bytes==0x1700u);
 PspModuleInfo module;
 assert(psp_module_discover(elf,sizeof elf,&cpu,&module)==0);
 assert(module.module_address==GUEST_BASE+0x900u);
 assert(!strcmp(module.name,"Synthetic145"));
 assert(module.import_top==GUEST_BASE+0x400u);
 assert(module.import_end==GUEST_BASE+0x400u+LIB_COUNT*20u);
 assert(module.export_top==GUEST_BASE+0x880u);
 PspPrxImportInventory inventory;
 assert(psp_prx_import_inventory(&cpu,module.import_top,
                                  module.import_end,&inventory)==0);
 assert(inventory.library_count==LIB_COUNT &&
        inventory.function_count==FUNC_COUNT);
 assert(!strcmp(inventory.libraries[0].name,"MockLib00"));
 assert(!strcmp(inventory.libraries[18].name,"MockLib18"));
 assert(inventory.libraries[0].function_count==7u);
 assert(inventory.libraries[18].function_count==19u);
 assert(inventory.libraries[18].first_nid==0x10000000u+126u);
 assert(inventory.libraries[18].nid_table==GUEST_BASE+0xa00u+126u*4u);
 assert(inventory.libraries[18].stub_table==GUEST_BASE+0xc80u+126u*8u);
 /* The inventory intentionally exceeds the 64-binding prototype linker;
  * no unimplemented firmware NIDs are patched or treated as success. */
 PspPrxImports limited;
 assert(psp_prx_parse_imports(&cpu,module.import_top,
                              module.import_end,&limited)==-2);
 assert(limited.count==0u);
 assert(psp_cpu_step(&cpu)==0 && cpu.gpr[8]==7u);
 /* Invalid pointer, truncated NID/stub, and malicious name must fail
  * read-only and clear the caller's output. */
 const uint32_t original=r32(ram+0x400u);
 w32(ram+0x400u,0xfffffff0u);
 memset(&inventory,0x5a,sizeof inventory);
 assert(psp_prx_import_inventory(&cpu,module.import_top,
                                  module.import_end,&inventory)==-2);
 assert(inventory.library_count==0u&&inventory.function_count==0u);
 w32(ram+0x400u,original);
 ram[0x700u]=27u;
 assert(psp_prx_import_inventory(&cpu,module.import_top,
                                  module.import_end,&inventory)==-2);
 assert(inventory.library_count==0u);
 ram[0x700u]='M';
 /* Validate 24-byte import header independently, including size hints. */
 w32(ram+0x580u,GUEST_BASE+0x700u);ram[0x588u]=6;
 w16(ram+0x58au,1u);
 w32(ram+0x58cu,GUEST_BASE+0xa00u);
 w32(ram+0x590u,GUEST_BASE+0xc80u);
 assert(psp_prx_import_inventory(&cpu,GUEST_BASE+0x580u,
                                  GUEST_BASE+0x598u,&inventory)==0);
 assert(inventory.library_count==1u&&inventory.function_count==1u);
 assert(psp_prx_import_inventory(&cpu,GUEST_BASE+0x580u,
                                  GUEST_BASE+0x594u,&inventory)==-2);
 /* .rodata.sceModuleInfo must be genuinely covered by PT_LOAD. */
 uint8_t *section=elf+0x1d00u+80u;
 const uint32_t section_address=r32(section+12);
 w32(section+12,0x1500u);
 assert(psp_module_discover(elf,sizeof elf,&cpu,&module)==-2);
 assert(module.module_address==0u);
 w32(section+12,section_address);
 assert(psp_module_discover(elf,sizeof elf,&cpu,&module)==0);
 puts("PASS: mapped PRX module info, 145 verified imports across 19 libraries, no fake HLE");
 return 0;
}
