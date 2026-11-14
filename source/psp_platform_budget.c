#include "psp_platform_budget.h"
/* Targets and caps intentionally favor correctness and headroom over
 * making unverified claims about PSP game compatibility.
 * The DS with RAM pack has exactly the same *hot* budget as DS, because
 * an external cartridge does not accelerate ARM9 nor grow internal RAM. */
#define KB(x) ((uint32_t)(x)*1024u)
static const PspPlatformBudget policies[PSP_TARGET_COUNT]={
 {PSP_TARGET_DS,"DS / DS Lite", KB(512),0,KB(256),KB(64),KB(64),KB(48),
  256,192,8,PSP_GRAPHICS_DS_TILES,0},
 {PSP_TARGET_DS_SLOT2,"DS + Slot-2 RAM",KB(512),KB(2048),KB(256),KB(64),KB(64),KB(48),
  256,192,8,PSP_GRAPHICS_DS_TILES,1},
 {PSP_TARGET_DSI,"DSi / DSi XL",KB(2048),0,KB(512),KB(128),KB(128),KB(48),
  256,192,8,PSP_GRAPHICS_DS_TILES,0},
 {PSP_TARGET_OLD3DS,"Original 3DS / XL",KB(8192),0,KB(1024),KB(2048),KB(256),KB(400),
  400,240,16,PSP_GRAPHICS_3DS_SPRITES,0},
 {PSP_TARGET_NEW3DS,"New 3DS / XL",KB(16384),0,KB(2048),KB(4096),KB(512),KB(400),
  400,240,16,PSP_GRAPHICS_3DS_SPRITES,0}
};
int psp_platform_budget_choose(PspTarget target,PspPlatformBudget *out){
 if(!out || (unsigned)target>=PSP_TARGET_COUNT)return -1;
 *out=policies[(unsigned)target];
 return 0;
}
int psp_platform_budget_frame_bytes(const PspPlatformBudget *b,
                                    uint8_t bpp,uint32_t *out) {
 if(!b||!out||(bpp!=4&&bpp!=8&&bpp!=16&&bpp!=24))return -1;
 uint64_t bits=(uint64_t)b->width*b->height*bpp;
 uint64_t bytes=(bits+7u)/8u;
 if(!b->width||!b->height||bytes>UINT32_MAX)return -1;
 *out=(uint32_t)bytes;
 return 0;
}
int psp_platform_budget_validate(const PspPlatformBudget *b){
 uint32_t bytes=0;
 if(!b||!b->name||!b->hot_ram_budget||!b->cache_budget||
    !b->texture_budget||!b->io_budget||
    psp_platform_budget_frame_bytes(b,b->recommended_bpp,&bytes))return -1;
 if(b->graphics==PSP_GRAPHICS_DS_TILES){
  if(b->width!=256||b->height!=192||b->recommended_bpp!=8 ||
     b->frame_buffer_bytes>96u*1024u)return -1;
 }else if(b->graphics==PSP_GRAPHICS_3DS_SPRITES){
  if(b->width!=400||b->height!=240||b->recommended_bpp!=16||
     b->frame_buffer_bytes<bytes)return -1;
 }else return -1;
 if((b->target==PSP_TARGET_DS_SLOT2)!=!!b->has_slot2 ||
    (!!b->cold_data_budget)!=!!b->has_slot2)return -1;
 if(b->target==PSP_TARGET_DS_SLOT2 &&
    b->hot_ram_budget!=policies[PSP_TARGET_DS].hot_ram_budget)return -1;
 if((unsigned)b->target>=PSP_TARGET_COUNT)return -1;
 /* Enforce actual retail DS and DSi main-memory ceilings, while keeping
  * existing conservative PSP3DS hot and cold data allocations. These
  * budgets are maximum independent reservations, not measured free heap
  * or a guarantee that the full sum can be allocated contiguously. */
 if(b->target<=PSP_TARGET_DSI){
  const uint64_t total=(uint64_t)b->hot_ram_budget+b->cache_budget+
      b->texture_budget+b->io_budget+b->frame_buffer_bytes;
  if(b->target==PSP_TARGET_DSI){
   if(b->hot_ram_budget>2048u*1024u ||
      b->cold_data_budget!=0u || total>16u*1024u*1024u)return -1;
  }else{
   if(b->hot_ram_budget>512u*1024u ||
      total>4u*1024u*1024u ||
      b->cold_data_budget>(b->target==PSP_TARGET_DS_SLOT2?
                            2048u*1024u:0u))return -1;
  }
 }
 return 0;
}
