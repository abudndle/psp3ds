#include "psp_prx_image.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define BASE UINT32_C(0x08800000)
static void w16(uint8_t *p,uint16_t v){
 p[0]=(uint8_t)v;p[1]=(uint8_t)(v>>8);
}
static void w32(uint8_t *p,uint32_t v){
 for(unsigned i=0;i<4;i++)p[i]=(uint8_t)(v>>(8u*i));
}
static uint32_t r32(const uint8_t *p){
 return (uint32_t)p[0]|(uint32_t)p[1]<<8|
        (uint32_t)p[2]<<16|(uint32_t)p[3]<<24;
}
static void fixture(uint8_t elf[512]){
 memset(elf,0,512);
 memcpy(elf,"\177ELF",4);elf[4]=1;elf[5]=1;elf[6]=1;
 w16(elf+16,0xffa0u);w16(elf+18,8u);
 w32(elf+20,1u);w32(elf+24,0u);w32(elf+28,52u);
 w16(elf+40,52u);w16(elf+42,32u);w16(elf+44,3u);
 /* Code, initialized bytes and BSS at relative address zero. */
 w32(elf+52,1u);w32(elf+56,256u);w32(elf+60,0u);
 w32(elf+64,0x00000030u); /* p_paddr is NOT a mapping base. */
 w32(elf+68,32u);w32(elf+72,48u);w32(elf+76,5u);
 /* Separate 16-byte data plus 16-byte BSS at relative address 64. */
 w32(elf+84,1u);w32(elf+88,320u);w32(elf+92,64u);
 w32(elf+100,16u);w32(elf+104,32u);w32(elf+108,6u);
 /* Two PSP R_MIPS_32 records, one per loaded segment. */
 w32(elf+116,0x700000a0u);w32(elf+120,384u);
 w32(elf+132,16u);w32(elf+136,16u);
 w32(elf+256,0x25080001u); /* addiu $t0,$t0,1 */
 w32(elf+260,64u); /* pointer relocated by load segment 0 delta */
 w32(elf+320,16u); /* pointer relocated by load segment 1 delta */
 w32(elf+384,4u);w32(elf+388,2u);
 w32(elf+392,0u);w32(elf+396,(1u<<8)|2u);
}
static void setup(PspCpu *cpu,uint8_t *guest,size_t size){
 memset(guest,0xa5,size);
 psp_cpu_init(cpu,guest,size,BASE,BASE+8u);
 cpu->gpr[8]=77u;
}
static void unchanged(const uint8_t *before,const uint8_t *ram,size_t n,
                      const PspCpu *saved,const PspCpu *cpu,
                      const PspPrxImageResult *result){
 assert(memcmp(before,ram,n)==0);
 assert(memcmp(saved,cpu,sizeof(*cpu))==0);
 assert(result->entry==0 && result->image_bytes==0 &&
        result->load_segments==0 && result->relocation_streams==0);
}
int main(void){
 uint8_t elf[512],ram[256],before[256];
 PspCpu cpu,saved;PspPrxImageResult result;
 fixture(elf);setup(&cpu,ram,sizeof ram);
 assert(psp_prx_image_map_relocated(elf,sizeof elf,&cpu,&result)==PSP_PRX_IMAGE_OK);
 assert(result.entry==BASE && result.image_bytes==96u);
 assert(result.load_segments==2u && result.relocation_streams==1u);
 assert(cpu.pc==BASE && cpu.next_pc==BASE+4u && cpu.gpr[8]==0);
 assert(r32(ram+4)==BASE+64u && r32(ram+64)==BASE+16u);
 for(unsigned i=32;i<48;i++)assert(ram[i]==0);
 for(unsigned i=80;i<96;i++)assert(ram[i]==0);
 for(unsigned i=48;i<64;i++)assert(ram[i]==0xa5);
 assert(ram[96]==0xa5);
 assert(psp_cpu_step(&cpu)==0 && cpu.gpr[8]==1u);
 /* An oversized mapping must not mutate any guest state. */
 fixture(elf);setup(&cpu,ram,64u);
 memcpy(before,ram,sizeof ram);saved=cpu;
 assert(psp_prx_image_map_relocated(elf,sizeof elf,&cpu,&result)==PSP_PRX_IMAGE_MAPPING);
 unchanged(before,ram,64u,&saved,&cpu,&result);
 /* Malformed relocation after copying sections rolls back all BSS,
  * segment copies, registers and return values. */
 fixture(elf);w32(elf+384,0xdeadbeefu);
 setup(&cpu,ram,sizeof ram);memcpy(before,ram,sizeof ram);saved=cpu;
 assert(psp_prx_image_map_relocated(elf,sizeof elf,&cpu,&result)==PSP_PRX_IMAGE_RELOCATION);
 unchanged(before,ram,sizeof ram,&saved,&cpu,&result);
 /* A forbidden PRX relocation type also rolls back the image. */
 fixture(elf);w32(elf+388,255u);
 setup(&cpu,ram,sizeof ram);memcpy(before,ram,sizeof ram);saved=cpu;
 assert(psp_prx_image_map_relocated(elf,sizeof elf,&cpu,&result)==PSP_PRX_IMAGE_RELOCATION);
 unchanged(before,ram,sizeof ram,&saved,&cpu,&result);
 /* Overlapping segments, invalid file offsets and nonexecutable entry
  * are all rejected before mutation. */
 fixture(elf);w32(elf+92,40u);
 setup(&cpu,ram,sizeof ram);memcpy(before,ram,sizeof ram);saved=cpu;
 assert(psp_prx_image_map_relocated(elf,sizeof elf,&cpu,&result)==PSP_PRX_IMAGE_MAPPING);
 unchanged(before,ram,sizeof ram,&saved,&cpu,&result);
 fixture(elf);w32(elf+88,510u);
 assert(psp_prx_image_map_relocated(elf,sizeof elf,&cpu,&result)==PSP_PRX_IMAGE_MAPPING);
 unchanged(before,ram,sizeof ram,&saved,&cpu,&result);
 fixture(elf);w32(elf+24,80u);
 assert(psp_prx_image_map_relocated(elf,sizeof elf,&cpu,&result)==PSP_PRX_IMAGE_MAPPING);
 unchanged(before,ram,sizeof ram,&saved,&cpu,&result);
 /* No PSP relocation stream: fail closed. */
 fixture(elf);w32(elf+116,0u);
 assert(psp_prx_image_map_relocated(elf,sizeof elf,&cpu,&result)==PSP_PRX_IMAGE_MAPPING);
 unchanged(before,ram,sizeof ram,&saved,&cpu,&result);
 fixture(elf);w16(elf+16,2u);
 assert(psp_prx_image_map_relocated(elf,sizeof elf,&cpu,&result)==PSP_PRX_IMAGE_FORMAT);
 unchanged(before,ram,sizeof ram,&saved,&cpu,&result);
 assert(psp_prx_image_map_relocated(NULL,sizeof elf,&cpu,&result)==PSP_PRX_IMAGE_ARGUMENT);
 assert(psp_prx_image_map_relocated(elf,sizeof elf,&cpu,NULL)==PSP_PRX_IMAGE_ARGUMENT);
 puts("PASS: relocate PSP ET_SCE_PRX ELF, BSS, MIPS entry and whole-image rollback");
 return 0;
}
