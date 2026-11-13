#include "psp_reloc2.h"
#include <limits.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

static uint16_t rd16(const uint8_t *p) { return (uint16_t)(p[0] | (uint16_t)p[1]<<8); }
static uint32_t rd32(const uint8_t *p) {
 return (uint32_t)p[0] | (uint32_t)p[1]<<8 | (uint32_t)p[2]<<16 | (uint32_t)p[3]<<24;
}
static void wr32(uint8_t *p,uint32_t v) {
 for(unsigned i=0;i<4;i++)p[i]=(uint8_t)(v>>(i*8));
}
static unsigned mask_bits(unsigned count) { return count==0 ? 0 : ((1u<<count)-1u); }
static int32_t sign_bits(uint32_t n,unsigned bits) {
 if(!bits) return 0;
 uint32_t sign=1u<<(bits-1);
 uint32_t mask=mask_bits(bits);
 n &=mask;
 return (int32_t)((n^sign)-sign);
}
int psp_reloc2_apply(PspCpu *cpu,const uint8_t *bytes,size_t length,
                     const PspRelocSegment *segments,size_t segment_count,
                     unsigned rel_segment_index) {
 if(!cpu || !bytes || !segments || !segment_count || segment_count>256 ||
    length<6 || length>524288 || rel_segment_index==0 || rel_segment_index>256) return -1;
 unsigned segbits=1;
 while ((1u<<segbits)<rel_segment_index) segbits++;
 unsigned flagbits=bytes[2], typebits=bytes[3];
 if(!flagbits || !typebits || flagbits>8 || typebits>8 ||
    flagbits+typebits+segbits>=16) return -2;
 size_t pos=4;
 unsigned nf=bytes[pos++];
 if(!nf || nf>256 || nf>length-pos) return -2;
 const uint8_t *flags=bytes+pos;pos+=nf;
 if(pos==length)return -2;
 unsigned nt=bytes[pos++];
 if(!nt || nt>256 || nt>length-pos)return -2;
 const uint8_t *types=bytes+pos;pos+=nt;
 size_t capacity=(length-pos)/2;
 if(capacity>65536 || capacity>SIZE_MAX/8)return -2;
 uint8_t *decoded=(uint8_t *)malloc(capacity ? capacity*8 : 1);
 if(!decoded)return -3;
 size_t produced=0;
 unsigned offseg=0;
 uint32_t base=0;
 int base_set=0;
 int status=-2;
 while(pos<length) {
  if(length-pos<2) goto done;
  uint32_t cmd=rd16(bytes+pos);pos+=2;
  unsigned fidx=cmd & mask_bits(flagbits);
  unsigned seg=(cmd>>flagbits)&mask_bits(segbits);
  unsigned tidx=(cmd>>(flagbits+segbits))&mask_bits(typebits);
  if(fidx>=nf || tidx>=nt || seg>=segment_count)goto done;
  unsigned flag=flags[fidx],type=types[tidx];
  unsigned offsetmode=flag&6u;
  if((flag&0xc0u) || offsetmode==6u)goto done;
  if((flag&1u)==0) {
   if((flag&0x38u) || (flag&~6u))goto done;
   offseg=seg;
   if(offsetmode==0) {
    base=cmd>>(flagbits+segbits);
   }else if(offsetmode==4) {
    if(length-pos<4)goto done;
    base=rd32(bytes+pos);pos+=4;
   }else goto done;
   base_set=1;
   continue;
  }
  if(!base_set || (flag&0x38u)!=0 || (type!=0 && type!=2 && type!=4))goto done;
  unsigned shift=flagbits+segbits+typebits;
  unsigned highbits=16-shift;
  int32_t delta;
  if(offsetmode==0) {
   delta=sign_bits(cmd>>shift,highbits);
   base+=(uint32_t)delta;
  }else if(offsetmode==2) {
   if(length-pos<2)goto done;
   delta=sign_bits(cmd>>shift,highbits);
   uint32_t extension=rd16(bytes+pos);pos+=2;
   base+=((uint32_t)delta<<16)|extension;
  }else if(offsetmode==4) {
   if(length-pos<4)goto done;
   base=rd32(bytes+pos);pos+=4;
  }else goto done;
  if(type==0)continue;
  if(produced>=capacity)goto done;
  wr32(decoded+produced*8,base);
  wr32(decoded+produced*8+4,(seg<<16)|(offseg<<8)|type);
  produced++;
 }
 status=psp_reloc_apply(cpu,decoded,produced*8,segments,segment_count);
done:
 free(decoded);
 return status;
}
