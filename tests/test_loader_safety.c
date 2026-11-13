#include "psp_loader.h"
#include "psp_core.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define BASE 0x08800000u
static void w16(uint8_t *p, unsigned x) { p[0]=(uint8_t)x; p[1]=(uint8_t)(x>>8); }
static void w32(uint8_t *p, uint32_t x) {
 for (int i=0;i<4;i++) p[i]=(uint8_t)(x>>(8*i));
}
static void fixture(uint8_t *elf) {
 memset(elf,0,512);
 memcpy(elf,"\177ELF",4);
 elf[4]=1;elf[5]=1;elf[6]=1;
 w16(elf+16,2);w16(elf+18,8);w32(elf+20,1);
 w32(elf+24,BASE);w32(elf+28,52);
 w16(elf+40,52);w16(elf+42,32);w16(elf+44,2);
 /* First mapped segment contains the entry instructions plus BSS. */
 w32(elf+52,1);w32(elf+56,192);w32(elf+60,BASE);
 w32(elf+68,16);w32(elf+72,32);w32(elf+76,5);
 /* Second disjoint data segment contains a separate BSS tail. */
 w32(elf+84,1);w32(elf+88,256);w32(elf+92,BASE+64);
 w32(elf+100,16);w32(elf+104,32);w32(elf+108,6);
 w32(elf+192,0x24080001u);
 w32(elf+256,0x12345678u);
}
static void reject(uint8_t *elf, PspLoadStatus expected) {
 uint8_t ram[160];PspLoadResult result;
 memset(ram,0xa5,sizeof ram);
 result.entry=123;result.segments=999;
 assert(psp_elf_load(elf,512,ram,sizeof ram,BASE,&result)==expected);
 assert(result.entry==0 && result.segments==0);
 for (size_t i=0;i<sizeof ram;i++) assert(ram[i]==0xa5);
}
int main(void) {
 uint8_t elf[512],ram[160];PspLoadResult result;
 fixture(elf);memset(ram,0xa5,sizeof ram);
 assert(psp_elf_load(elf,sizeof elf,ram,sizeof ram,BASE,&result)==PSP_LOAD_OK);
 assert(result.entry==BASE && result.segments==2);
 assert(ram[0]==1 && ram[1]==0 && ram[2]==8 && ram[3]==0x24);
 for (size_t i=16;i<32;i++) assert(ram[i]==0);
 assert(ram[64]==0x78 && ram[65]==0x56 && ram[66]==0x34 && ram[67]==0x12);
 for (size_t i=80;i<96;i++) assert(ram[i]==0);
 assert(ram[32]==0xa5 && ram[96]==0xa5);
 /* The second segment's file-backed portion overlaps the first BSS. */
 fixture(elf);w32(elf+92,BASE+16);
 reject(elf,PSP_LOAD_BAD_SEGMENT);
 /* Overlap in either ordering must reject before RAM mutation. */
 fixture(elf);w32(elf+60,BASE+72);w32(elf+24,BASE+72);
 reject(elf,PSP_LOAD_BAD_SEGMENT);
 /* The entry point points into zero-filled executable BSS. */
 fixture(elf);w32(elf+24,BASE+20);
 reject(elf,PSP_LOAD_ENTRY_INVALID);
 /* Entry must cover one complete four-byte MIPS instruction. */
 fixture(elf);w32(elf+24,BASE+14);
 reject(elf,PSP_LOAD_ENTRY_INVALID);
 fixture(elf);w32(elf+24,BASE+1);
 reject(elf,PSP_LOAD_ENTRY_INVALID);
 /* Zero-length memory mappings may not count toward a valid image. */
 fixture(elf);w32(elf+104,0);
 reject(elf,PSP_LOAD_BAD_SEGMENT);
 /* Adjacent BSS/data intervals are allowed. */
 fixture(elf);w32(elf+92,BASE+32);
 memset(ram,0xa5,sizeof ram);
 assert(psp_elf_load(elf,sizeof elf,ram,sizeof ram,BASE,&result)==PSP_LOAD_OK);
 assert(result.segments==2 && ram[32]==0x78 && ram[31]==0);
 puts("PASS: disjoint segments, atomic rejection, BSS entry and instruction alignment");
 return 0;
}
