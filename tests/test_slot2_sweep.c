#include "psp_slot2_probe.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Synthetic host fixture only. This is NOT a physical expansion pack test. */
int main(void) {
 const uint32_t bytes=8u*1024u*1024u;
 uint16_t *mem=(uint16_t *)malloc(bytes);
 uint16_t *before=(uint16_t *)malloc(bytes);
 assert(mem && before);
 for(size_t i=0;i<bytes/2u;i++)
  mem[i]=(uint16_t)(i^(i>>13)^0x9a35u);
 memcpy(before,mem,bytes);
 PspSlot2Result r=psp_slot2_probe_samples(mem,bytes,1,1,32);
 assert(r.status==PSP_SLOT2_RW_VERIFIED);
 assert(r.probed_locations==32 && r.detected_bytes==bytes);
 assert(memcmp(mem,before,bytes)==0);
 r=psp_slot2_probe_samples(mem,bytes,1,1,64);
 assert(r.status==PSP_SLOT2_RW_VERIFIED && r.probed_locations==64);
 assert(memcmp(mem,before,bytes)==0);
 r=psp_slot2_probe_samples(mem,bytes,0,1,32);
 assert(r.status==PSP_SLOT2_UNAVAILABLE && r.probed_locations==0);
 r=psp_slot2_probe_samples(NULL,bytes,1,1,32);
 assert(r.status==PSP_SLOT2_UNVERIFIED && r.probed_locations==0);
 r=psp_slot2_probe_samples(mem,bytes,1,0,32);
 assert(r.status==PSP_SLOT2_UNVERIFIED && r.probed_locations==0);
 r=psp_slot2_probe_samples(mem,bytes,1,1,0);
 assert(r.status==PSP_SLOT2_UNVERIFIED && r.probed_locations==0);
 r=psp_slot2_probe_samples(mem,bytes,1,1,1);
 assert(r.status==PSP_SLOT2_UNVERIFIED && r.probed_locations==0);
 r=psp_slot2_probe_samples(mem,bytes,1,1,65);
 assert(r.status==PSP_SLOT2_UNVERIFIED && r.probed_locations==0);
 r=psp_slot2_probe_samples(mem,bytes-1,1,1,32);
 assert(r.status==PSP_SLOT2_UNVERIFIED && r.probed_locations==0);
 assert(memcmp(mem,before,bytes)==0);
 free(before);free(mem);
 puts("PASS: 8 MiB sampled RAM sweep, 32/64 locations, exact restoration, guarded failures");
 return 0;
}
