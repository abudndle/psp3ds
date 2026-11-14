#include <stddef.h>
#include <stdint.h>
#include "psp_rom_pager.h"

/* Original synthetic ROM fixture, not a PSP game or mounted cartridge file.
 * The same PspRomPageRead callback can later read from NitroFS or another
 * verified cartridge file source without allocating the entire archive. */
typedef struct {uint32_t calls;} RomFixture;
static uint8_t fixture_byte(uint32_t at){
 return (uint8_t)((at>>12u)*37u+(at&255u)+((at>>8u)&15u));
}
static int fixture_read(void *ctx,uint32_t off,uint8_t *dst,size_t bytes){
 RomFixture *f=(RomFixture*)ctx;
 if(!f || !dst || !bytes || bytes>PSP_ROM_PAGER_PAGE_BYTES ||
    (uint64_t)off+bytes>PSP_ROM_PAGER_MAX_IMAGE)return -1;
 f->calls++;
 for(size_t i=0;i<bytes;i++)dst[i]=fixture_byte(off+(uint32_t)i);
 return 0;
}
static uint8_t hot_demo_pages[2u*PSP_ROM_PAGER_PAGE_BYTES];
static uint8_t pak_hot_page[PSP_ROM_PAGER_PAGE_BYTES];
static uint16_t pak_saved_words[2u*PSP_ROM_PAGER_PAGE_BYTES/2u];

int psp_nds_rom_pager_synthetic_test(void){
 PspRomPager p;RomFixture source={0};uint8_t bytes[8];
 const uint32_t last=PSP_ROM_PAGER_MAX_IMAGE-8u;
 if(psp_rom_pager_init(&p,hot_demo_pages,2,PSP_ROM_PAGER_MAX_IMAGE,
                       fixture_read,&source,NULL,0)!=0)return -1;
 if(psp_rom_pager_read(&p,last,bytes,8)!=0)return -1;
 for(unsigned i=0;i<8;i++)
  if(bytes[i]!=fixture_byte(last+i))return -1;
 if(psp_rom_pager_read(&p,4094u,bytes,8)!=0)return -1;
 for(unsigned i=0;i<8;i++)
  if(bytes[i]!=fixture_byte(4094u+i))return -1;
 return p.stats.source_reads==3u?0:-1;
}

/* This is invoked only while the BlocksDS peripheral is OPEN, after the
 * existing read/write and 32-location memory verification. The whole 8KiB
 * test range is saved and restored. No PSP executable is mapped here. */
int psp_nds_rom_pager_verified_slot2_test(volatile uint16_t *region,
                                           uint32_t capacity){
 if(!region || capacity<sizeof pak_saved_words)return -1;
 const size_t words=sizeof pak_saved_words/sizeof pak_saved_words[0];
 for(size_t i=0;i<words;i++)pak_saved_words[i]=region[i];
 PspGuestWindow window;
 PspRomPager p;RomFixture source={0};uint8_t bytes[4];
 int ok=psp_guest_window_init(&window,region,0x09000000u,
                             sizeof pak_saved_words,1,1)==0;
 if(ok)ok=psp_rom_pager_init(&p,pak_hot_page,1,
                     PSP_ROM_PAGER_MAX_IMAGE,fixture_read,&source,
                     &window,2)==0;
 if(ok)ok=psp_rom_pager_read(&p,0u,bytes,4)==0;
 if(ok)ok=psp_rom_pager_read(&p,4096u,bytes,4)==0;
 if(ok)ok=psp_rom_pager_read(&p,0u,bytes,4)==0;
 if(ok){
  for(unsigned i=0;i<4;i++)if(bytes[i]!=fixture_byte(i))ok=0;
  if(p.stats.cold_hits!=1u || source.calls!=2u)ok=0;
 }
 /* Always restore everything written to the verified memory expansion. */
 for(size_t i=0;i<words;i++)region[i]=pak_saved_words[i];
 for(size_t i=0;i<words;i++)if(region[i]!=pak_saved_words[i])ok=0;
 return ok?0:-1;
}
