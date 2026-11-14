#include "psp_rom_pager.h"
#include <stdint.h>
#include <string.h>

static uint32_t use_clock(PspRomPager *p){
 if(p->clock==UINT32_MAX){
  /* Rebase ages before a wrap; ordering is preserved until ties,
   * which merely change the chosen LRU victim, not cached contents. */
  p->clock>>=1;
  for(unsigned i=0;i<p->hot_count;i++)p->hot_ages[i]>>=1;
  for(unsigned i=0;i<p->cold_count;i++)p->cold_ages[i]>>=1;
 }
 return ++p->clock;
}
static unsigned hot_victim(const PspRomPager *p){
 unsigned target=0;
 for(unsigned i=0;i<p->hot_count;i++){
  if(!p->hot_valid[i])return i;
  if(p->hot_ages[i]<p->hot_ages[target])target=i;
 }
 return target;
}
static unsigned cold_victim(const PspRomPager *p){
 unsigned target=0;
 for(unsigned i=0;i<p->cold_count;i++){
  if(!p->cold_valid[i])return i;
  if(p->cold_ages[i]<p->cold_ages[target])target=i;
 }
 return target;
}
int psp_rom_pager_init(PspRomPager *p,uint8_t *hot,unsigned hot_count,
                       uint32_t image_bytes,PspRomPageRead reader,
                       void *opaque,const PspGuestWindow *cold,
                       unsigned cold_count){
 if(p)memset(p,0,sizeof(*p));
 if(!p || !hot || !reader || !image_bytes ||
    image_bytes>PSP_ROM_PAGER_MAX_IMAGE ||
    !hot_count || hot_count>PSP_ROM_PAGER_MAX_HOT ||
    cold_count>PSP_ROM_PAGER_MAX_COLD ||
    (cold_count && (!cold || !cold->enabled || !cold->words ||
                    cold->capacity<cold_count*PSP_ROM_PAGER_PAGE_BYTES ||
                    (uint64_t)cold->guest_base +
                       cold_count*PSP_ROM_PAGER_PAGE_BYTES >
                       UINT32_MAX+UINT64_C(1))) ||
    (!cold_count && cold))return -1;
 p->hot_bytes=hot;p->hot_count=(uint8_t)hot_count;
 p->cold_count=(uint8_t)cold_count;
 p->image_bytes=image_bytes;
 p->read_source=reader;p->opaque=opaque;
 if(cold_count)p->cold_window=*cold;
 return 0;
}
static int cold_restore(PspRomPager *p,unsigned slot,uint8_t *hot){
 const uint64_t address=(uint64_t)p->cold_window.guest_base +
                         (uint64_t)slot*PSP_ROM_PAGER_PAGE_BYTES;
 for(unsigned i=0;i<PSP_ROM_PAGER_PAGE_BYTES;i+=2u){
  uint16_t value;
  if(psp_guest_window_read16(&p->cold_window,(uint32_t)(address+i),&value)!=0)
   return -1;
  hot[i]=(uint8_t)value;hot[i+1]=(uint8_t)(value>>8);
 }
 return 0;
}
static int cold_store(PspRomPager *p,unsigned slot,const uint8_t *hot){
 const uint64_t address=(uint64_t)p->cold_window.guest_base+
                         (uint64_t)slot*PSP_ROM_PAGER_PAGE_BYTES;
 for(unsigned i=0;i<PSP_ROM_PAGER_PAGE_BYTES;i+=2u){
  const uint16_t value=(uint16_t)(hot[i] | (uint16_t)hot[i+1]<<8);
  if(psp_guest_window_write16(&p->cold_window,(uint32_t)(address+i),value)!=0)
   return -1;
 }
 return 0;
}
static int fetch(PspRomPager *p,uint32_t page,uint8_t **data){
 for(unsigned i=0;i<p->hot_count;i++){
  if(p->hot_valid[i] && p->hot_tags[i]==page){
   p->hot_ages[i]=use_clock(p);p->stats.hot_hits++;
   *data=p->hot_bytes+(size_t)i*PSP_ROM_PAGER_PAGE_BYTES;
   return 0;
  }
 }
 const unsigned victim=hot_victim(p);
 uint8_t *dst=p->hot_bytes+(size_t)victim*PSP_ROM_PAGER_PAGE_BYTES;
 /* Invalidate the victim before overwriting it. */
 p->hot_valid[victim]=0;
 for(unsigned i=0;i<p->cold_count;i++){
  if(p->cold_valid[i] && p->cold_tags[i]==page){
   if(cold_restore(p,i,dst)!=0){p->cold_valid[i]=0;return -1;}
   p->cold_ages[i]=use_clock(p);p->stats.cold_hits++;
   p->hot_tags[victim]=page;p->hot_valid[victim]=1;
   p->hot_ages[victim]=use_clock(p);*data=dst;
   return 0;
  }
 }
 const uint64_t off=(uint64_t)page*PSP_ROM_PAGER_PAGE_BYTES;
 if(off>=p->image_bytes)return -1;
 const size_t length=(size_t)(p->image_bytes-off)<PSP_ROM_PAGER_PAGE_BYTES?
                     (size_t)(p->image_bytes-off):PSP_ROM_PAGER_PAGE_BYTES;
 memset(dst,0,PSP_ROM_PAGER_PAGE_BYTES);
 p->stats.source_misses++;
 if(p->read_source(p->opaque,(uint32_t)off,dst,length)!=0)return -1;
 p->stats.source_reads++;
 if(p->cold_count){
  const unsigned target=cold_victim(p);
  p->cold_valid[target]=0;
  if(cold_store(p,target,dst)!=0)return -1;
  p->cold_tags[target]=page;p->cold_valid[target]=1;
  p->cold_ages[target]=use_clock(p);
 }
 p->hot_tags[victim]=page;p->hot_valid[victim]=1;
 p->hot_ages[victim]=use_clock(p);*data=dst;
 return 0;
}
int psp_rom_pager_read(PspRomPager *p,uint32_t off,
                        uint8_t *dst,size_t length){
 if(!p || !dst || !p->hot_bytes || !p->read_source ||
    !p->hot_count || !length || length>PSP_ROM_PAGER_MAX_READ ||
    (uint64_t)off+length>p->image_bytes)return -1;
 uint8_t staged[PSP_ROM_PAGER_MAX_READ];
 size_t copied=0;
 while(copied<length){
  const uint64_t absolute=(uint64_t)off+copied;
  const uint32_t page=(uint32_t)(absolute/PSP_ROM_PAGER_PAGE_BYTES);
  const unsigned within=(unsigned)(absolute%PSP_ROM_PAGER_PAGE_BYTES);
  const size_t remain=length-copied;
  const size_t amount=remain<PSP_ROM_PAGER_PAGE_BYTES-within?
       remain:PSP_ROM_PAGER_PAGE_BYTES-within;
  uint8_t *bytes;
  if(fetch(p,page,&bytes)!=0)return -1;
  memcpy(staged+copied,bytes+within,amount);
  copied+=amount;
 }
 memcpy(dst,staged,length);
 return 0;
}
int psp_rom_pager_read32(PspRomPager *p,uint32_t off,uint32_t *out){
 uint8_t bytes[4];
 if(!out || psp_rom_pager_read(p,off,bytes,sizeof bytes)!=0)return -1;
 *out=(uint32_t)bytes[0]|(uint32_t)bytes[1]<<8|
      (uint32_t)bytes[2]<<16|(uint32_t)bytes[3]<<24;
 return 0;
}
