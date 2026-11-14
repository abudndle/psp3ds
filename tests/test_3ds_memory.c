#include "psp_3ds_memory.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#define MB PSP_3DS_MIB
int main(void){
 Psp3dsMemoryPlan old,new3,legacy,maximum,low;
 assert(psp_3ds_memory_plan(PSP_TARGET_OLD3DS,64u*MB,60u*MB,&old)==0);
 assert(old.installed_ram_bytes==128u*MB);
 assert(old.typical_application_bytes==64u*MB);
 assert(old.special_application_upper_bytes==96u*MB);
 assert(old.observed_free_bytes==60u*MB);
 assert(old.preserve_for_system_bytes==16u*MB);
 assert(old.permitted_guest_bytes==44u*MB);
 assert(old.recommended_guest_bytes==32u*MB);
 assert(old.full_psp_guest_fits && old.jetpack_prx_fits);
 assert(old.potential_extra_cache_bytes==8u*MB);

 assert(psp_3ds_memory_plan(PSP_TARGET_NEW3DS,124u*MB,110u*MB,&new3)==0);
 assert(new3.installed_ram_bytes==256u*MB);
 assert(new3.typical_application_bytes==124u*MB);
 assert(new3.special_application_upper_bytes==178u*MB);
 assert(new3.preserve_for_system_bytes==24u*MB);
 assert(new3.permitted_guest_bytes==86u*MB);
 assert(new3.recommended_guest_bytes==64u*MB);
 assert(new3.potential_extra_cache_bytes==22u*MB);
 assert(new3.full_psp_guest_fits && new3.jetpack_prx_fits);
 assert(new3.recommended_guest_bytes>old.recommended_guest_bytes);

 /* Running New3DS in legacy application memory mode is not a free
  * 124/178 MiB allocation entitlement. Use observed free RAM. */
 assert(psp_3ds_memory_plan(PSP_TARGET_NEW3DS,64u*MB,54u*MB,&legacy)==0);
 assert(legacy.observed_application_bytes==64u*MB);
 assert(legacy.recommended_guest_bytes==30u*MB);
 assert(!legacy.full_psp_guest_fits && legacy.jetpack_prx_fits);

 assert(psp_3ds_memory_plan(PSP_TARGET_NEW3DS,178u*MB,160u*MB,&maximum)==0);
 assert(maximum.recommended_guest_bytes==64u*MB);
 assert(maximum.observed_application_bytes==178u*MB);
 assert(psp_3ds_memory_plan(PSP_TARGET_OLD3DS,96u*MB,90u*MB,&maximum)==0);
 assert(maximum.recommended_guest_bytes==32u*MB);

 assert(psp_3ds_memory_plan(PSP_TARGET_OLD3DS,64u*MB,17u*MB,&low)==0);
 assert(low.permitted_guest_bytes==1u*MB);
 assert(low.recommended_guest_bytes==0u);
 assert(!low.jetpack_prx_fits && !low.full_psp_guest_fits);
 assert(psp_3ds_memory_plan(PSP_TARGET_NEW3DS,124u*MB,31u*MB,&low)==0);
 assert(low.recommended_guest_bytes==0u && !low.jetpack_prx_fits);
 assert(psp_3ds_memory_plan(PSP_TARGET_OLD3DS,64u*MB,64u*MB+1u,&low)==-1);
 assert(low.installed_ram_bytes==0u);
 assert(psp_3ds_memory_plan(PSP_TARGET_NEW3DS,257u*MB,100u*MB,&low)==-1);
 assert(psp_3ds_memory_plan(PSP_TARGET_DS,4u*MB,3u*MB,&low)==-1);
 assert(psp_3ds_memory_plan(PSP_TARGET_DSI,16u*MB,15u*MB,&low)==-1);
 assert(psp_3ds_memory_plan(PSP_TARGET_NEW3DS,124u*MB,70u*MB,NULL)==-1);
 puts("PASS: measured-memory old/New 3DS policy, legacy and extended app modes, conservative PSP guest caps");
 return 0;
}
