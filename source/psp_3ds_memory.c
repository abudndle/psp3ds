#include "psp_3ds_memory.h"
#include <string.h>

#define JETPACK_IMAGE_BYTES 6320916u
static uint32_t min32(uint32_t a,uint32_t b){return a<b?a:b;}
int psp_3ds_memory_plan(PspTarget target,uint32_t region,
                        uint32_t free_bytes,Psp3dsMemoryPlan *out){
 if(out)memset(out,0,sizeof(*out));
 if(!out||(target!=PSP_TARGET_OLD3DS&&target!=PSP_TARGET_NEW3DS))
  return -1;
 const int new_model=target==PSP_TARGET_NEW3DS;
 const uint32_t physical=new_model?PSP_3DS_NEW_PHYSICAL:PSP_3DS_OLD_PHYSICAL;
 const uint32_t typical=new_model?PSP_3DS_NEW_STANDARD_APP:
                                  PSP_3DS_OLD_STANDARD_APP;
 const uint32_t special=new_model?PSP_3DS_NEW_MAX_SPECIAL_APP:
                                  PSP_3DS_OLD_MAX_SPECIAL_APP;
 const uint32_t reserve=new_model?PSP_3DS_NEW_HEADROOM:PSP_3DS_OLD_HEADROOM;
 const uint32_t target_cap=new_model?PSP_3DS_NEW_PSP_GUEST_CAP:
                                     PSP_3DS_OLD_PSP_GUEST_CAP;
 if(region<PSP_3DS_MIN_GUEST || region>physical || free_bytes>region)
  return -1;
 out->target=target;
 out->installed_ram_bytes=physical;
 out->typical_application_bytes=typical;
 out->special_application_upper_bytes=special;
 out->observed_application_bytes=region;
 out->observed_free_bytes=free_bytes;
 out->preserve_for_system_bytes=reserve;
 const uint32_t spendable=free_bytes>reserve?free_bytes-reserve:0u;
 const uint32_t capacity=min32(spendable,target_cap);
 /* Page-align guest allocations to 1 MiB so fallback attempts keep a
  * conservative bounded size on devices with fragmented heaps. */
 const uint32_t usable=(capacity/PSP_3DS_MIB)*PSP_3DS_MIB;
 if(usable>=PSP_3DS_MIN_GUEST)out->recommended_guest_bytes=usable;
 out->permitted_guest_bytes=spendable;
 out->full_psp_guest_fits=usable>=target_cap;
 out->jetpack_prx_fits=out->recommended_guest_bytes>=JETPACK_IMAGE_BYTES;
 if(spendable>out->recommended_guest_bytes){
  const uint32_t extra=spendable-out->recommended_guest_bytes;
  out->potential_extra_cache_bytes=min32(extra,new_model?
         24u*PSP_3DS_MIB:8u*PSP_3DS_MIB);
 }
 return 0;
}
