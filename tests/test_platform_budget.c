#include "psp_platform_budget.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
int main(void){
 PspPlatformBudget all[PSP_TARGET_COUNT];
 for(int i=0;i<PSP_TARGET_COUNT;i++){
  assert(psp_platform_budget_choose((PspTarget)i,&all[i])==0);
  assert(all[i].target==i);
  assert(psp_platform_budget_validate(&all[i])==0);
  uint32_t bytes=0;
  assert(psp_platform_budget_frame_bytes(&all[i],all[i].recommended_bpp,&bytes)==0);
  assert(bytes>0);
 }
 assert(all[PSP_TARGET_DS].hot_ram_budget==all[PSP_TARGET_DS_SLOT2].hot_ram_budget);
 assert(all[PSP_TARGET_DS].cold_data_budget==0);
 assert(all[PSP_TARGET_DS_SLOT2].cold_data_budget>0);
 assert(all[PSP_TARGET_DSI].has_slot2==0);
 assert(all[PSP_TARGET_OLD3DS].graphics==PSP_GRAPHICS_3DS_SPRITES);
 assert(all[PSP_TARGET_NEW3DS].hot_ram_budget>all[PSP_TARGET_OLD3DS].hot_ram_budget);
 uint32_t bytes;
 assert(psp_platform_budget_frame_bytes(&all[PSP_TARGET_DS],8,&bytes)==0&&bytes==49152u);
 assert(psp_platform_budget_frame_bytes(&all[PSP_TARGET_DS],16,&bytes)==0&&bytes==98304u);
 assert(psp_platform_budget_frame_bytes(&all[PSP_TARGET_OLD3DS],16,&bytes)==0&&bytes==192000u);
 assert(psp_platform_budget_frame_bytes(&all[PSP_TARGET_NEW3DS],24,&bytes)==0&&bytes==288000u);
 assert(psp_platform_budget_frame_bytes(&all[0],3,&bytes)==-1);
 assert(psp_platform_budget_choose(PSP_TARGET_COUNT,&all[0])==-1);
 assert(psp_platform_budget_choose(PSP_TARGET_DS,NULL)==-1);
 PspPlatformBudget bad=all[PSP_TARGET_DS_SLOT2];bad.has_slot2=0;
 assert(psp_platform_budget_validate(&bad)==-1);
 bad=all[PSP_TARGET_DS];bad.cold_data_budget=2048;
 assert(psp_platform_budget_validate(&bad)==-1);
 bad=all[PSP_TARGET_NEW3DS];bad.frame_buffer_bytes=1;
 assert(psp_platform_budget_validate(&bad)==-1);
 puts("PASS: per-model RAM/graphics software policy, frame sizes, fail-closed flags");
 return 0;
}
