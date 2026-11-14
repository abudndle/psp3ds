#include <3ds.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "psp_3ds_memory.h"
#include "psp_memory_guard.h"

/* Real device diagnostics. The APPLICATION-region kernel counters are
 * observed data, not promised malloc capacity or PSP game frame rates.
 * Keep linear GPU heap and the OS/application runtime headroom untouched. */
int psp_3ds_native_memory_probe(PspTarget model){
 const u32 region=osGetMemRegionSize(MEMREGION_APPLICATION);
 const u32 used=osGetMemRegionUsed(MEMREGION_APPLICATION);
 if(!region || used>region){
  printf("3DS application RAM: unavailable\n");
  return -1;
 }
 Psp3dsMemoryPlan p;
 if(psp_3ds_memory_plan(model,region,region-used,&p)!=0){
  printf("3DS memory policy: invalid OS report\n");
  return -1;
 }
 printf("3DS FCRAM installed: %lu MiB\n",
        (unsigned long)(p.installed_ram_bytes/PSP_3DS_MIB));
 printf("APP region/free: %lu / %lu MiB\n",
        (unsigned long)(p.observed_application_bytes/PSP_3DS_MIB),
        (unsigned long)(p.observed_free_bytes/PSP_3DS_MIB));
 printf("PSP guest target: %lu MiB\n",
        (unsigned long)(p.recommended_guest_bytes/PSP_3DS_MIB));
 printf("Reserve: %lu MiB (policy)\n",
        (unsigned long)(p.preserve_for_system_bytes/PSP_3DS_MIB));
 /* Probe whether the process heap can really provide a contiguous region.
  * This DOES NOT allocate or retain PSP emulated RAM for gameplay.
  * Reduce by halves on failure, keep at least 8 MiB target to be useful
  * for small ELF diagnostics. Never assert device FPS or PSP boot. */
 PspMemoryGuardPolicy quota;
 PspMemoryGuard guard={0};
 PspMemoryGuardReport session_report={0};
 if(psp_memory_guard_policy_3ds(&p,&quota)!=0 ||
    psp_memory_guard_init(&guard,&quota,NULL,NULL,NULL)!=0){
  printf("3DS guarded PSP RAM: insufficient headroom\n");
  return -1;
 }
 uint32_t wanted=p.recommended_guest_bytes;
 uint32_t confirmed=0;
 while(wanted>=PSP_3DS_MIN_GUEST){
  void *owned=NULL;
  const int allocated=psp_memory_guard_acquire(&guard,PSP_MEMORY_GUEST,
                                               wanted,&owned);
  if(allocated==0){
   volatile uint8_t *touch=(volatile uint8_t*)owned;
   for(uint32_t i=0;i<wanted;i+=4096u)touch[i]=(uint8_t)(i>>12u);
   touch[wanted-1u]=0xa5u;
   confirmed=wanted;
   if(psp_memory_guard_release(&guard,owned)!=0)confirmed=0;
   break;
  }
  wanted=(wanted/2u/PSP_3DS_MIB)*PSP_3DS_MIB;
 }
 if(psp_memory_guard_close(&guard,&session_report)!=0 ||
    session_report.reclaimed_allocations||
    session_report.end_outstanding_bytes){
  printf("3DS PSP session cleanup: FAILED\n");
  return -1;
 }
 printf("3DS PSP session cleanup: PASS (synthetic)\n");
 printf("malloc guest probe: %lu MiB %s\n",
        (unsigned long)(confirmed/PSP_3DS_MIB),
        confirmed?"(allocated/released)":"(unavailable)");
 printf("Jetpack PRX 6.32MB: %s\n",
        confirmed>=6320916u?"fits probe only":"not confirmed");
 return confirmed?0:-1;
}
