#include "psp_slot2_probe.h"

/* Keep this routine OS-agnostic so the exact probe can be tested on Linux.
 * Only a trusted platform adapter may supply a live hardware address. */
PspSlot2Result psp_slot2_probe(volatile uint16_t *base,uint32_t bytes,
                               int ram_confirmed,int single_bank) {
 PspSlot2Result out={PSP_SLOT2_UNAVAILABLE,0,0};
 if(!ram_confirmed)return out;
 out.status=PSP_SLOT2_UNVERIFIED;
 if(!base || !single_bank || bytes<4096u || bytes>32u*1024u*1024u ||
    (bytes & 1u))return out;
 out.detected_bytes=bytes;
 size_t words=bytes/sizeof(uint16_t);
 size_t positions[2]={0,words/2u};
 if(!positions[1] || positions[1]==positions[0])return out;
 const uint16_t patterns[2]={UINT16_C(0xa55a),UINT16_C(0x5aa5)};
 for(unsigned i=0;i<2;i++) {
  volatile uint16_t *location=base+positions[i];
  uint16_t original=*location;
  uint16_t test=(uint16_t)(original^patterns[i]);
  *location=test;
  uint16_t seen=*location;
  *location=original;
  uint16_t restored=*location;
  if(seen!=test || restored!=original)return out;
  out.probed_locations++;
 }
 out.status=PSP_SLOT2_RW_VERIFIED;
 return out;
}
