#include "psp_rom_pager.h"
#include "psp_platform_budget.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

typedef struct {
 uint32_t image_bytes;
 uint32_t failed_page;
 uint32_t calls;
 uint32_t last_offset;
 size_t last_length;
} FakeRom;
static uint8_t pattern(uint32_t offset){
 return (uint8_t)((offset>>12u)*37u+(offset&255u)+((offset>>8u)&15u));
}
static int read_file(void *ctx,uint32_t at,uint8_t *dst,size_t n){
 FakeRom *r=(FakeRom*)ctx;
 if(!r||!dst||!n||n>PSP_ROM_PAGER_PAGE_BYTES||
    (uint64_t)at+n>r->image_bytes)return -1;
 r->calls++;r->last_offset=at;r->last_length=n;
 if(at/PSP_ROM_PAGER_PAGE_BYTES==r->failed_page){
  dst[0]=0x99; /* simulate a callback that writes before failing */
  return -1;
 }
 for(size_t i=0;i<n;i++)dst[i]=pattern(at+(uint32_t)i);
 return 0;
}
static void expected(const uint8_t *p,uint32_t at,size_t n){
 for(size_t i=0;i<n;i++)assert(p[i]==pattern(at+(uint32_t)i));
}
static void host_hot_and_sparse_512mb(void){
 uint8_t hot[2*PSP_ROM_PAGER_PAGE_BYTES]={0};
 FakeRom source={PSP_ROM_PAGER_MAX_IMAGE,UINT32_MAX,0,0,0};
 PspRomPager cache;
 assert(psp_rom_pager_init(&cache,hot,2,source.image_bytes,
                           read_file,&source,NULL,0)==0);
 uint8_t bytes[PSP_ROM_PAGER_MAX_READ];
 memset(bytes,0x55,sizeof bytes);
 assert(psp_rom_pager_read(&cache,4094u,bytes,8)==0);
 expected(bytes,4094u,8);
 assert(source.calls==2u && cache.stats.source_reads==2u);
 assert(psp_rom_pager_read(&cache,4095u,bytes,5)==0);
 expected(bytes,4095u,5);
 assert(source.calls==2u && cache.stats.hot_hits>=2u);
 /* Simulated 512-MB ROM offset: no memory allocation proportional to ROM. */
 const uint32_t last=PSP_ROM_PAGER_MAX_IMAGE-4u;
 assert(psp_rom_pager_read(&cache,last,bytes,4)==0);
 expected(bytes,last,4u);
 assert(source.last_offset==PSP_ROM_PAGER_MAX_IMAGE-4096u);
 uint32_t word=0;
 assert(psp_rom_pager_read32(&cache,last,&word)==0);
 assert(word==((uint32_t)pattern(last) |
              (uint32_t)pattern(last+1u)<<8u |
              (uint32_t)pattern(last+2u)<<16u |
              (uint32_t)pattern(last+3u)<<24u));
 memset(bytes,0x55,sizeof bytes);
 assert(psp_rom_pager_read(&cache,PSP_ROM_PAGER_MAX_IMAGE-2u,
                           bytes,4)==-1);
 for(unsigned i=0;i<sizeof bytes;i++)assert(bytes[i]==0x55u);
 assert(psp_rom_pager_read(&cache,0,bytes,33)==-1);
 assert(psp_rom_pager_read(&cache,0,NULL,1)==-1);
 assert(psp_rom_pager_read32(&cache,0,NULL)==-1);
}
static void cold_memory_pak(void){
 uint8_t hot[PSP_ROM_PAGER_PAGE_BYTES]={0};
 uint16_t slot2[2*PSP_ROM_PAGER_PAGE_BYTES/2u];
 memset(slot2,0xa5,sizeof slot2);
 PspGuestWindow window;
 assert(psp_guest_window_init(&window,slot2,0x09000000u,
                              sizeof slot2,1,1)==0);
 FakeRom source={PSP_ROM_PAGER_MAX_IMAGE,UINT32_MAX,0,0,0};
 PspRomPager p;
 assert(psp_rom_pager_init(&p,hot,1,source.image_bytes,
                           read_file,&source,&window,2)==0);
 uint8_t bytes[8];
 assert(psp_rom_pager_read(&p,0u,bytes,4)==0);
 assert(psp_rom_pager_read(&p,4096u,bytes,4)==0);
 assert(psp_rom_pager_read(&p,8192u,bytes,4)==0);
 assert(source.calls==3 && p.stats.source_reads==3);
 assert(psp_rom_pager_read(&p,4096u,bytes,4)==0);
 expected(bytes,4096u,4);
 assert(p.stats.cold_hits==1u && source.calls==3u);
 assert(psp_rom_pager_read(&p,4096u,bytes,4)==0);
 assert(p.stats.hot_hits>=1u);
 assert(psp_rom_pager_read(&p,0u,bytes,4)==0);
 assert(source.calls==4u); /* LRU evicted cold page 0 */
 assert(p.stats.source_misses==4u);
 /* Selected Slot-2 data is stored as 16-bit words, not executable code. */
 uint16_t first;
 assert(psp_guest_window_read16(&window,0x09000000u,&first)==0);
 assert(first!=(uint16_t)0xa5a5u);
 /* A real Pak must be independently verified and OPEN. */
 PspGuestWindow unchecked={0};
 assert(psp_rom_pager_init(&p,hot,1,source.image_bytes,
                           read_file,&source,&unchecked,1)==-1);
 assert(psp_rom_pager_init(&p,hot,1,source.image_bytes,
                           read_file,&source,&window,3)==-1);
 assert(psp_rom_pager_init(&p,hot,1,source.image_bytes,
                           read_file,&source,NULL,1)==-1);
}
static void bounds_and_failure(void){
 uint8_t hot[PSP_ROM_PAGER_PAGE_BYTES]={0};
 FakeRom source={4099u,UINT32_MAX,0,0,0};
 PspRomPager p;
 assert(psp_rom_pager_init(&p,hot,1,source.image_bytes,
                           read_file,&source,NULL,0)==0);
 uint8_t bytes[8]={0};
 assert(psp_rom_pager_read(&p,4095u,bytes,4)==0);
 expected(bytes,4095u,4);
 assert(source.last_offset==4096u && source.last_length==3u);
 assert(psp_rom_pager_read(&p,4096u,bytes,4)==-1);
 memset(bytes,0x5a,sizeof bytes);
 source.failed_page=0u;
 assert(psp_rom_pager_read(&p,0u,bytes,4)==-1);
 for(unsigned i=0;i<sizeof bytes;i++)assert(bytes[i]==0x5au);
 source.failed_page=UINT32_MAX;
 assert(psp_rom_pager_read(&p,0u,bytes,4)==0);
 expected(bytes,0u,4);
 assert(psp_rom_pager_init(&p,hot,0,source.image_bytes,
                           read_file,&source,NULL,0)==-1);
 assert(psp_rom_pager_init(&p,hot,PSP_ROM_PAGER_MAX_HOT+1u,
                           source.image_bytes,read_file,&source,NULL,0)==-1);
 assert(psp_rom_pager_init(&p,hot,1,
                           PSP_ROM_PAGER_MAX_IMAGE+1u,read_file,&source,NULL,0)==-1);
 assert(psp_rom_pager_init(&p,hot,1,0,read_file,&source,NULL,0)==-1);
 assert(psp_rom_pager_init(NULL,hot,1,4096,read_file,&source,NULL,0)==-1);
 assert(psp_rom_pager_init(&p,NULL,1,4096,read_file,&source,NULL,0)==-1);
 assert(psp_rom_pager_init(&p,hot,1,4096,NULL,&source,NULL,0)==-1);
}
static void target_budgets(void){
 for(int t=0;t<PSP_TARGET_COUNT;t++){
  PspPlatformBudget b;
  assert(psp_platform_budget_choose((PspTarget)t,&b)==0);
  assert(psp_platform_budget_validate(&b)==0);
  assert(PSP_ROM_PAGER_MAX_HOT*PSP_ROM_PAGER_PAGE_BYTES<b.cache_budget);
 }
 PspPlatformBudget ds,pak;
 assert(psp_platform_budget_choose(PSP_TARGET_DS,&ds)==0);
 assert(psp_platform_budget_choose(PSP_TARGET_DS_SLOT2,&pak)==0);
 assert(ds.hot_ram_budget==pak.hot_ram_budget);
 assert(pak.cold_data_budget>=
        PSP_ROM_PAGER_MAX_COLD*PSP_ROM_PAGER_PAGE_BYTES);
 assert(!ds.has_slot2 && pak.has_slot2);
}
int main(void){
 host_hot_and_sparse_512mb();cold_memory_pak();
 bounds_and_failure();target_budgets();
 puts("PASS: sparse 512 MB cartridge ROM, small LRU hot cache, verified Slot-2 cold tier");
 return 0;
}
