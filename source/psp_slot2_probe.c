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

/* Scan a sparse distribution across the *reported* single bank. Every
 * store restores its exact previous value before the next sample.
 * One point may fail while still returning its count of prior successes,
 * but status remains unverified. It is NEVER a guest-memory allocator. */
PspSlot2Result psp_slot2_probe_samples(volatile uint16_t *base,
                                       uint32_t bytes,
                                       int ram_confirmed,
                                       int single_bank,
                                       unsigned samples) {
 PspSlot2Result out={PSP_SLOT2_UNAVAILABLE,0,0};
 if(!ram_confirmed)return out;
 out.status=PSP_SLOT2_UNVERIFIED;
 if(!base || !single_bank || bytes<4096u ||
    bytes>32u*1024u*1024u || (bytes&1u) ||
    samples<2u || samples>PSP_SLOT2_MAX_SAMPLES)return out;
 size_t words=(size_t)bytes/2u;
 if(words<(size_t)samples)return out;
 out.detected_bytes=bytes;
 for(unsigned i=0;i<samples;i++){
  /* The final sample reaches the very end of the reported bank. */
  size_t pos=(size_t)(((uint64_t)(words-1u)*i)/(samples-1u));
  volatile uint16_t *loc=base+pos;
  uint16_t previous=*loc;
  uint16_t mask=(uint16_t)(UINT16_C(0x6d4b)+(uint16_t)(i*UINT16_C(0x0101)));
  uint16_t candidate=(uint16_t)(previous^mask);
  *loc=candidate;
  uint16_t observed=*loc;
  *loc=previous;
  uint16_t after=*loc;
  if(observed!=candidate || after!=previous)return out;
  out.probed_locations++;
 }
 out.status=PSP_SLOT2_RW_VERIFIED;
 return out;
}
