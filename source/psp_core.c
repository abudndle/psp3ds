#include "psp_core.h"
#include <string.h>
static uint16_t r16(const uint8_t *p) { return (uint16_t)(p[0] | (uint16_t)p[1]<<8); }
static uint32_t r32(const uint8_t *p) { return (uint32_t)p[0] | (uint32_t)p[1]<<8 | (uint32_t)p[2]<<16 | (uint32_t)p[3]<<24; }
int psp_inspect(const uint8_t *d, size_t n, PspFileInfo *o) {
  if (!d || !o) return -1;
  memset(o, 0, sizeof(*o));
  if (n>=40 && memcmp(d,"\0PBP",4)==0) {
    o->type=PSP_FILE_PBP; o->version=r32(d+4);
    for (unsigned i=0;i<8;i++) o->offsets[i]=r32(d+8+i*4);
    if (o->offsets[0]<40 || o->offsets[7]>n) return -2;
    for (unsigned i=1;i<8;i++) if (o->offsets[i]<o->offsets[i-1] || o->offsets[i]>n) return -2;
    return 0;
  }
  if (n>=52 && memcmp(d,"\177ELF",4)==0) {
    if (d[4]!=1 || d[5]!=1 || r16(d+18)!=8) return -2;
    o->type=PSP_FILE_ELF; o->machine=r16(d+18); o->entry=r32(d+24); return 0;
  }
  if (n>=24 && memcmp(d,"CISO",4)==0) { o->type=PSP_FILE_CSO; return 0; }
  if (n>=0x8006 && memcmp(d+0x8001,"CD001",5)==0) { o->type=PSP_FILE_ISO; return 0; }
  return -3;
}
static uint8_t *translate(PspCpu *c,uint32_t addr,size_t bytes) {
  if (c->edram && addr>=PSP_GE_EDRAM_BASE && addr-PSP_GE_EDRAM_BASE<PSP_GE_EDRAM_SIZE) {
    size_t offset=addr-PSP_GE_EDRAM_BASE;
    size_t size=c->edram_size<PSP_GE_EDRAM_SIZE ? c->edram_size : PSP_GE_EDRAM_SIZE;
    if (offset<=size && bytes<=size-offset) return c->edram+offset;
    return NULL;
  }
  if (addr<c->ram_base) return NULL;
  uint64_t offset=(uint64_t)addr-c->ram_base;
  if (offset>c->ram_size || bytes>c->ram_size-(size_t)offset) return NULL;
  return c->ram+(size_t)offset;
}
void psp_cpu_init(PspCpu *c,uint8_t *ram,size_t ram_size,uint32_t base,uint32_t start) {
  memset(c,0,sizeof(*c));c->ram=ram;c->ram_size=ram_size;c->ram_base=base;c->pc=start;c->next_pc=start+4;
}
static uint32_t asr32(uint32_t value, unsigned shift) {
  if (!shift) return value;
  return (value >> shift) | ((value & 0x80000000u) ? (~0u << (32 - shift)) : 0u);
}
static int16_t signed16(uint16_t v) {
  return (int16_t)(v < 0x8000u ? (int32_t)v : (int32_t)v - 65536);
}
static int64_t signed32(uint32_t v) {
  return v < 0x80000000u ? (int64_t)v : (int64_t)v - 4294967296LL;
}
static int trap(PspCpu *c, unsigned code, uint32_t info) {
  c->ll_valid=0;
  c->last_trap_code=info;
  c->cp0_epc=c->delay_slot ? c->pc-4u : c->pc;
  c->cp0_cause=(c->cp0_cause & ~0x8000007cu) | ((code & 31u)<<2)
              | (c->delay_slot ? 0x80000000u : 0u);
  c->cp0_status|=2u;
  c->fault=-3;
  return -3;
}
static int sum_overflow(uint32_t a, uint32_t b, uint32_t out) {
  return ((~(a^b) & (a^out)) & 0x80000000u)!=0u;
}
static int diff_overflow(uint32_t a, uint32_t b, uint32_t out) {
  return (((a^b) & (a^out)) & 0x80000000u)!=0u;
}
static void store16(uint8_t *p, uint32_t v) { p[0]=(uint8_t)v; p[1]=(uint8_t)(v>>8); }
static void store32(uint8_t *p, uint32_t v) { store16(p,v); store16(p+2,v>>16); }
/* Decode binary32 directly so conversion is independent of host FP modes. */
static uint32_t trunc_single_to_word(uint32_t bits) {
 unsigned exponent=(bits>>23)&255u;
 uint32_t fraction=bits&0x7fffffu;
 int negative=(bits>>31)!=0;
 if (exponent==255u && fraction) return 0x7fffffffu;
 if (exponent<127u) return 0;
 if (exponent>=158u) return negative ? 0x80000000u : 0x7fffffffu;
 unsigned shift=exponent-127u;
 uint32_t significand=fraction|0x800000u;
 uint32_t magnitude=shift>=23 ? significand<<(shift-23) : significand>>(23-shift);
 return negative ? 0u-magnitude : magnitude;
}
int psp_cpu_step(PspCpu *c) {
  if (!c || !c->ram || c->halted) return -2;
  uint8_t *ptr;
  if ((c->pc & 3u) || !(ptr=translate(c,c->pc,4))) { c->fault=-2; return -2; }
  uint32_t instr=r32(ptr),op=instr>>26,rs=(instr>>21)&31,rt=(instr>>16)&31;
  uint32_t rd=(instr>>11)&31,sh=(instr>>6)&31,fn=instr&63;
  uint32_t uimm=instr&65535u;
  int32_t imm=(int32_t)signed16((uint16_t)uimm);
  uint32_t target=c->next_pc+(uint32_t)(imm*4);
  uint32_t branch=0; int taken=0, likely=0, status=0, has_delay=0;
  uint32_t a=c->gpr[rs],b=c->gpr[rt],addr;
  switch(op) {
    case 0:
      switch(fn) {
        case 0: c->gpr[rd]=b<<sh; break;
        case 2: c->gpr[rd]=b>>sh; break;
        case 3: c->gpr[rd]=asr32(b,sh); break;
        case 4: c->gpr[rd]=b<<(a&31u); break;
        case 6: c->gpr[rd]=b>>(a&31u); break;
        case 7: c->gpr[rd]=asr32(b,a&31u); break;
        case 10: if (!b) c->gpr[rd]=a; break; /* MOVZ */
        case 11: if (b) c->gpr[rd]=a; break; /* MOVN */
        case 12: return trap(c,8,(instr>>6)&0xfffffu);
        case 13: return trap(c,9,(instr>>6)&0xfffffu);
        case 32: {uint32_t value=a+b; if (sum_overflow(a,b,value)) return trap(c,12,0); c->gpr[rd]=value;break;}
        case 34: {uint32_t value=a-b; if (diff_overflow(a,b,value)) return trap(c,12,0); c->gpr[rd]=value;break;}
        case 8: branch=a; taken=1; break;
        case 9: c->gpr[rd?rd:31]=c->next_pc+4; branch=a; taken=1; break;
        case 16: c->gpr[rd]=c->hi; break;
        case 17: c->hi=a; break;
        case 18: c->gpr[rd]=c->lo; break;
        case 19: c->lo=a; break;
        case 24: {int64_t product=signed32(a)*signed32(b); c->lo=(uint32_t)product; c->hi=(uint32_t)((uint64_t)product>>32); break;}
        case 25: {uint64_t product=(uint64_t)a*b; c->lo=(uint32_t)product; c->hi=(uint32_t)(product>>32); break;}
        case 26:
          if(b) {int64_t divisor=signed32(b);int64_t dividend=signed32(a);
            c->lo=(uint32_t)(dividend/divisor);c->hi=(uint32_t)(dividend%divisor);} break;
        case 27: if(b) {c->lo=a/b;c->hi=a%b;} break;
        case 33: c->gpr[rd]=a+b; break;
        case 35: c->gpr[rd]=a-b; break;
        case 36: c->gpr[rd]=a&b; break;
        case 37: c->gpr[rd]=a|b; break;
        case 38: c->gpr[rd]=a^b; break;
        case 39: c->gpr[rd]=~(a|b); break;
        case 42: c->gpr[rd]=signed32(a)<signed32(b); break;
        case 43: c->gpr[rd]=a<b; break;
        default: status=-1;
      } break;
    case 1:
      if (rt==0 || rt==1 || rt==2 || rt==3 || rt==16 || rt==17 || rt==18 || rt==19) {
        taken=(rt==0 || rt==2 || rt==16 || rt==18) ? (signed32(a)<0) : (signed32(a)>=0);
        likely=(rt==2 || rt==3 || rt==18 || rt==19);
        branch=target;
        if ((rt==16 || rt==17 || rt==18 || rt==19) && taken) c->gpr[31]=c->next_pc+4;
      } else status=-1;
      break;
    case 2: branch=(c->next_pc&0xf0000000u)|((instr&0x03ffffffu)<<2);taken=1;break;
    case 3: c->gpr[31]=c->next_pc+4; branch=(c->next_pc&0xf0000000u)|((instr&0x03ffffffu)<<2);taken=1;break;
    case 4: taken=a==b;branch=target;break;
    case 5: taken=a!=b;branch=target;break;
    case 6: taken=signed32(a)<=0;branch=target;break;
    case 7: taken=signed32(a)>0;branch=target;break;
    case 20: taken=a==b;branch=target;likely=1;break;
    case 21: taken=a!=b;branch=target;likely=1;break;
    case 22: taken=signed32(a)<=0;branch=target;likely=1;break;
    case 23: taken=signed32(a)>0;branch=target;likely=1;break;
    case 8: {uint32_t value=a+(uint32_t)imm; if (sum_overflow(a,(uint32_t)imm,value)) return trap(c,12,0); c->gpr[rt]=value;break;}
    case 9: c->gpr[rt]=a+(uint32_t)imm;break;
    case 10: c->gpr[rt]=signed32(a)<imm;break;
    case 11: c->gpr[rt]=a<(uint32_t)imm;break;
    case 12: c->gpr[rt]=a&uimm;break;
    case 13: c->gpr[rt]=a|uimm;break;
    case 14: c->gpr[rt]=a^uimm;break;
    case 15: c->gpr[rt]=uimm<<16;break;
    case 16: {
      uint32_t *reg=NULL;
      switch(rd) {
        case 8: reg=&c->cp0_badvaddr; break;
        case 12: reg=&c->cp0_status; break;
        case 13: reg=&c->cp0_cause; break;
        case 14: reg=&c->cp0_epc; break;
        default: status=-1; break;
      }
      if (reg && (instr&7u)==0) {
        if (rs==0) c->gpr[rt]=*reg;
        else if (rs==4 && rd==12) *reg=b;
        else status=-1;
      } else status=-1;
      break;
    }
    case 17: /* COP1 register transfers preserve all 32 bits. */
      if (rs==16 && rt==0 && fn==6) c->fpr[sh]=c->fpr[rd]; /* MOV.S */
      else if (rs==16 && rt==0 && fn==13) c->fpr[sh]=trunc_single_to_word(c->fpr[rd]);
      else if ((instr & 0x7ffu) != 0) status=-1;
      else if (rs==0) c->gpr[rt]=c->fpr[rd];
      else if (rs==4) c->fpr[rd]=b;
      else if (rs==2 && rd==31) c->gpr[rt]=c->fcr31;
      else if (rs==6 && rd==31) c->fcr31=b & 0x0181ffffu;
      else status=-1;
      break;
    case 49: case 57: /* LWC1 / SWC1 */
      addr=a+(uint32_t)imm;
      if ((addr&3u) || !(ptr=translate(c,addr,4))) status=-2;
      else if (op==49) c->fpr[rt]=r32(ptr);
      else store32(ptr,c->fpr[rt]);
      break;
    case 32: case 33: case 35: case 36: case 37:
      addr=a+(uint32_t)imm;
      if (((op==33 || op==37) && (addr&1u)) || (op==35 && (addr&3u)) ||
          !(ptr=translate(c,addr,op==35?4:((op==33 || op==37)?2:1)))) status=-2;
      else if(op==32) c->gpr[rt]=(uint32_t)(int32_t)(int8_t)*ptr;
      else if(op==36) c->gpr[rt]=*ptr;
      else if(op==35) c->gpr[rt]=r32(ptr);
      else {uint16_t val=r16(ptr);c->gpr[rt]=(op==33)?(uint32_t)(int32_t)signed16(val):val;}
      break;
    case 34: case 38:
      addr=a+(uint32_t)imm;
      {
        uint32_t aligned=addr&~3u, k=addr&3u;
        uint32_t merged=b;
        unsigned first=(op==34)?0:k, last=(op==34)?k:3;
        if (!(ptr=translate(c,aligned+first,last-first+1))) {status=-2;break;}
        for (unsigned i=first;i<=last;i++) {
          unsigned dest=(op==34)?(3-k+i):(i-k);
          merged=(merged&~(0xffu<<(8*dest))) | ((uint32_t)ptr[i-first]<<(8*dest));
        }
        c->gpr[rt]=merged;
      }
      break;
    case 48: /* LL */
      addr=a+(uint32_t)imm;
      if ((addr&3u) || !(ptr=translate(c,addr,4))) status=-2;
      else {
        c->gpr[rt]=r32(ptr);
        c->ll_address=addr;
        c->ll_valid=1;
      }
      break;
    case 56: /* SC */
      addr=a+(uint32_t)imm;
      if ((addr&3u) || !(ptr=translate(c,addr,4))) status=-2;
      else {
        int success=c->ll_valid && c->ll_address==addr;
        if (success) store32(ptr,b);
        c->gpr[rt]=(uint32_t)success;
      }
      c->ll_valid=0;
      break;
    case 42: case 46:
      addr=a+(uint32_t)imm;
      {
        uint32_t aligned=addr&~3u, k=addr&3u;
        unsigned first=(op==42)?0:k, last=(op==42)?k:3;
        if (!(ptr=translate(c,aligned+first,last-first+1))) {status=-2;break;}
        for (unsigned i=first;i<=last;i++) {
          unsigned src=(op==42)?(3-k+i):(i-k);
          ptr[i-first]=(uint8_t)(b>>(8*src));
        }
      }
      break;
    case 40: case 41: case 43:
      addr=a+(uint32_t)imm;
      if (((op==41) && (addr&1u)) || (op==43 && (addr&3u)) ||
          !(ptr=translate(c,addr,op==43?4:(op==41?2:1)))) status=-2;
      else if(op==40) *ptr=(uint8_t)b;
      else if(op==41) store16(ptr,b);
      else store32(ptr,b);
      break;
    default: status=-1;
  }
  if (!status && (op==1 || op==2 || op==3 || op==4 || op==5 || op==6 || op==7 ||
      op==20 || op==21 || op==22 || op==23 || (op==0 && (fn==8 || fn==9))))
    has_delay=1;
  if (!status && (op==40 || op==41 || op==42 || op==43 || op==46 || op==57) &&
      (addr&~3u)==c->ll_address) c->ll_valid=0;
  c->gpr[0]=0;
  if (status) {c->fault=status;return status;}
  if (likely && !taken) {
    c->pc=c->next_pc+4;
    c->next_pc=c->pc+4;
    c->delay_slot=0;
  } else {
    c->pc=c->next_pc;
    c->next_pc=taken?branch:c->next_pc+4;
    c->delay_slot=has_delay;
  }
  return 0;
}
