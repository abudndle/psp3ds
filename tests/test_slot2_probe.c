#include "psp_slot2_probe.h"
#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

int main(void) {
 const uint32_t bytes=8192;
 uint16_t *ram=(uint16_t*)malloc(bytes);
 uint16_t *copy=(uint16_t*)malloc(bytes);
 assert(ram&&copy);
 for(size_t i=0;i<bytes/2;i++)ram[i]=(uint16_t)(i^0x2345u);
 memcpy(copy,ram,bytes);
 PspSlot2Result r=psp_slot2_probe(ram,bytes,1,1);
 assert(r.status==PSP_SLOT2_RW_VERIFIED);
 assert(r.detected_bytes==bytes&&r.probed_locations==2);
 assert(memcmp(ram,copy,bytes)==0);
 r=psp_slot2_probe(ram,bytes,0,1);
 assert(r.status==PSP_SLOT2_UNAVAILABLE&&r.probed_locations==0);
 r=psp_slot2_probe(NULL,bytes,1,1);
 assert(r.status==PSP_SLOT2_UNVERIFIED);
 r=psp_slot2_probe(ram,bytes,1,0);
 assert(r.status==PSP_SLOT2_UNVERIFIED&&r.probed_locations==0);
 r=psp_slot2_probe(ram,2,1,1);
 assert(r.status==PSP_SLOT2_UNVERIFIED);
 r=psp_slot2_probe(ram,32u*1024u*1024u+2u,1,1);
 assert(r.status==PSP_SLOT2_UNVERIFIED);
 assert(memcmp(ram,copy,bytes)==0);
 free(copy);free(ram);
 puts("PASS: conservative Slot-2 probe, intact RAM, fail-closed gating");
 return 0;
}
