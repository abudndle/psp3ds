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
static int translate(PspCpu *c,uint32_t addr, size_t bytes,size_t *off) {
  if (addr<c->ram_base) return -1;
  uint64_t x=(uint64_t)addr-c->ram_base;
  if (x>c->ram_size || bytes>c->ram_size-(size_t)x) return -1;
  *off=(size_t)x;return 0;
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

/* ---- Single-precision MIPS COP1 (FPU) support. No libm or fenv dependency. ----
 * Rounding mode (fcr31 bits 1:0): 0 nearest-even, 1 toward zero, 2 toward +inf, 3 toward -inf.
 * Arithmetic is computed in host round-to-nearest and then corrected by one ulp using the
 * exactly computed error term, so directed modes do not depend on host FPU state. */
static float f_of(uint32_t v) { float f; memcpy(&f,&v,4); return f; }
static uint32_t bits_of(float f) { uint32_t v; memcpy(&v,&f,4); return v; }
static float f_trunc(float f) {
  if (!(f>-8388608.0f && f<8388608.0f)) return f; /* already integral, inf or NaN */
  return (float)(int32_t)f;
}
static float f_round(float f, int mode) {
  if (!(f>-8388608.0f && f<8388608.0f)) return f;
  float t=f_trunc(f), d=f-t;
  switch (mode) {
    case 1: return t;
    case 2: return d>0.0f ? t+1.0f : t;
    case 3: return d<0.0f ? t-1.0f : t;
    default:
      if (d>0.5f  || (d==0.5f  && (((int32_t)t)&1))) return t+1.0f;
      if (d<-0.5f || (d==-0.5f && (((int32_t)t)&1))) return t-1.0f;
      return t;
  }
}
static uint32_t f_to_w(float f, int mode) {
  if (f!=f) return 0x7fffffffu;
  float r=f_round(f,mode);
  if (r>=2147483648.0f) return 0x7fffffffu;
  if (r<-2147483648.0f) return 0x80000000u;
  return (uint32_t)(int32_t)r;
}
/* rb = round-to-nearest result bits; dir = sign of (exact - r). Returns the result for `mode`. */
static uint32_t round_adjust(uint32_t rb, int dir, int mode) {
  if (mode==0 || dir==0) return rb;
  int neg=(int)(rb>>31);
  int want_up = (mode==2) ? 1 : (mode==3) ? 0 : neg;
  if (want_up && dir>0)  return neg ? rb-1u : rb+1u;   /* next value up   */
  if (!want_up && dir<0) return neg ? rb+1u : rb-1u;   /* next value down */
  return rb;
}
/* Result when a finite operation overflowed to infinity under a directed mode. */
static uint32_t overflow_bits(uint32_t rb, int mode) {
  int neg=(int)(rb>>31);
  if (mode==1 || (mode==2 && neg) || (mode==3 && !neg)) return (rb&0x80000000u)|0x7f7fffffu;
  return rb;
}
static int isnanf_(float f) { return f!=f; }
static int isinff_(float f) { return (bits_of(f)&0x7fffffffu)==0x7f800000u; }
/* Finish a binary op: result r (host nearest), exact-error sign dir. */
static uint32_t finish2(float r, float x, float y, int dir, int mode) {
  if (isnanf_(r)) return (isnanf_(x)||isnanf_(y)) ? bits_of(r) : 0x7fc00000u; /* PSP makes +qNaN */
  if (isinff_(r) && !isinff_(x) && !isinff_(y)) return overflow_bits(bits_of(r),mode);
  return round_adjust(bits_of(r),dir,mode);
}
static int sgn_d(double d) { return d>0.0 ? 1 : (d<0.0 ? -1 : 0); }
static uint32_t f_add(float x, float y, int mode) {
  float s=x+y, bb=s-x, e=(x-(s-bb))+(y-bb);
  /* Exact cancellation of opposite signs is +0, but -0 when rounding toward -inf. */
  if (s==0.0f && ((bits_of(x)^bits_of(y))>>31)) return mode==3 ? 0x80000000u : 0u;
  return finish2(s,x,y,(e>0.0f)-(e<0.0f),mode);
}
static uint32_t f_mul(float x, float y, int mode) {
  float p=x*y;
  return finish2(p,x,y,sgn_d((double)x*(double)y-(double)p),mode);
}
static uint32_t f_div(float x, float y, int mode) {
  float q=x/y;
  double rem=(double)x-(double)q*(double)y;
  int dir=sgn_d(rem)*((bits_of(y)>>31)?-1:1);
  return finish2(q,x,y,dir,mode);
}
/* IEEE-754 binary32 square root with the given rounding mode, using integers. */
static uint32_t f_sqrt(float x, int mode) {
  uint32_t b=bits_of(x);
  if ((b&0x7fffffffu)==0 || (b&0x7fffffffu)>0x7f800000u) return b; /* +-0, NaN */
  if (b>>31) return 0x7fc00000u;                                    /* negative */
  if (b==0x7f800000u) return b;                                     /* +inf */
  int e=(int)((b>>23)&255u); uint32_t m=b&0x7fffffu;
  if (e==0) { e=1; while (!(m&0x800000u)) { m<<=1; e--; } } else m|=0x800000u;
  int ex=e-127;
  if (ex&1) { m<<=1; ex--; }
  uint64_t n=(uint64_t)m<<23, r=0, bit=1ull<<46;
  while (bit>n) bit>>=2;
  while (bit) { if (n>=r+bit) { n-=r+bit; r=(r>>1)+bit; } else r>>=1; bit>>=2; }
  if (mode==0 ? (n>r) : (mode==2 && n)) r++;
  ex/=2;
  if (r>>24) { r>>=1; ex++; }
  return ((uint32_t)(ex+127)<<23)|(uint32_t)(r&0x7fffffu);
}
/* int32 -> float honoring the rounding mode. */
static uint32_t w_to_f(uint32_t w, int mode) {
  int neg=(int)(w>>31);
  uint32_t u=neg ? 0u-w : w;
  if (!u) return 0;
  int hb=31; while (!((u>>hb)&1u)) hb--;
  int E=hb;                      /* exponent of the leading bit */
  uint32_t m;
  if (hb<=23) m=u<<(23-hb);      /* exact */
  else {
    int sh=hb-23;
    uint32_t low=u&((1u<<sh)-1u), half=1u<<(sh-1);
    m=u>>sh;
    int up = mode==0 ? (low>half || (low==half && (m&1u)))
           : mode==2 ? (!neg && low!=0) : mode==3 ? (neg && low!=0) : 0;
    if (up) m++;
    if (m>>24) { m>>=1; E++; }
  }
  return ((uint32_t)neg<<31) | ((uint32_t)(127+E)<<23) | (m&0x7fffffu);
}
static int cop1_single(PspCpu *c, uint32_t fn, uint32_t fs, uint32_t ft, uint32_t fd) {
  float x=f_of(c->fpr[fs]), y=f_of(c->fpr[ft]);
  int mode=(int)(c->fcr31&3u);
  switch (fn) {
    case 0: c->fpr[fd]=f_add(x,y,mode); break;
    case 1: c->fpr[fd]=f_add(x,-y,mode); break;
    case 2: c->fpr[fd]=f_mul(x,y,mode); break;
    case 3: c->fpr[fd]=f_div(x,y,mode); break;
    case 4: c->fpr[fd]=f_sqrt(x,mode); break;
    case 5: c->fpr[fd]=c->fpr[fs]&0x7fffffffu; break;
    case 6: c->fpr[fd]=c->fpr[fs]; break;
    case 7: c->fpr[fd]=c->fpr[fs]^0x80000000u; break;
    case 12: c->fpr[fd]=f_to_w(x,0); break;
    case 13: c->fpr[fd]=f_to_w(x,1); break;
    case 14: c->fpr[fd]=f_to_w(x,2); break;
    case 15: c->fpr[fd]=f_to_w(x,3); break;
    case 36: c->fpr[fd]=f_to_w(x,mode); break;
    default:
      if (fn>=48) {
        int un=(x!=x)||(y!=y), lt=!un&&x<y, eq=!un&&x==y;
        uint32_t cond=fn&15u;
        int r=((cond&1u)&&un)||((cond&2u)&&eq)||((cond&4u)&&lt);
        c->fcr31=(c->fcr31&~0x00800000u)|(r?0x00800000u:0u);
        break;
      }
      return -1;
  }
  return 0;
}
int psp_cpu_step(PspCpu *c) {
  if (!c || !c->ram || c->halted) return -2;
  size_t off;
  if ((c->pc & 3u) || translate(c,c->pc,4,&off)) { c->fault=-2; return -2; }
  uint32_t instr=r32(c->ram+off),op=instr>>26,rs=(instr>>21)&31,rt=(instr>>16)&31;
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
    case 17: {
      switch (rs) {
        case 0: c->gpr[rt]=c->fpr[rd]; break;                       /* MFC1 */
        case 2:                                                     /* CFC1 */
          if (rd==31) c->gpr[rt]=c->fcr31;
          else if (rd==0) c->gpr[rt]=0x00003351u;                   /* FIR (PSP FPU revision) */
          else if (rd>=25 && rd<=28) c->gpr[rt]=0;
          else status=-1;
          break;
        case 4: c->fpr[rd]=b; break;                                /* MTC1 */
        case 6: if (rd==31) c->fcr31=b&0x0183ffffu; else status=-1; break; /* CTC1 */
        case 8:                                                     /* BC1F/T/FL/TL */
          if (rt>3) { status=-1; break; }
          taken=(((c->fcr31>>23)&1u)==(rt&1u)); likely=(int)((rt>>1)&1u); branch=target;
          break;
        case 16: status=cop1_single(c,fn,rd,rt,sh); break;          /* fmt S */
        case 20: if (fn==32) c->fpr[sh]=w_to_f(c->fpr[rd],(int)(c->fcr31&3u)); else status=-1; break; /* CVT.S.W */
        default: status=-1;
      }
      break;
    }
    case 49: case 57:                                               /* LWC1 / SWC1 */
      addr=a+(uint32_t)imm;
      if ((addr&3u) || translate(c,addr,4,&off)) status=-2;
      else if (op==49) c->fpr[rt]=r32(c->ram+off);
      else store32(c->ram+off,c->fpr[rt]);
      break;
    case 32: case 33: case 35: case 36: case 37:
      addr=a+(uint32_t)imm;
      if (((op==33 || op==37) && (addr&1u)) || (op==35 && (addr&3u)) ||
          translate(c,addr,op==35?4:((op==33 || op==37)?2:1),&off)) status=-2;
      else if(op==32) c->gpr[rt]=(uint32_t)(int32_t)(int8_t)c->ram[off];
      else if(op==36) c->gpr[rt]=c->ram[off];
      else if(op==35) c->gpr[rt]=r32(c->ram+off);
      else {uint16_t val=r16(c->ram+off);c->gpr[rt]=(op==33)?(uint32_t)(int32_t)signed16(val):val;}
      break;
    case 34: case 38:
      addr=a+(uint32_t)imm;
      {
        uint32_t aligned=addr&~3u, k=addr&3u;
        uint32_t merged=b;
        unsigned first=(op==34)?0:k, last=(op==34)?k:3;
        if (translate(c,aligned+first,last-first+1,&off)) {status=-2;break;}
        for (unsigned i=first;i<=last;i++) {
          unsigned dest=(op==34)?(3-k+i):(i-k);
          merged=(merged&~(0xffu<<(8*dest))) | ((uint32_t)c->ram[off+i-first]<<(8*dest));
        }
        c->gpr[rt]=merged;
      }
      break;
    case 42: case 46:
      addr=a+(uint32_t)imm;
      {
        uint32_t aligned=addr&~3u, k=addr&3u;
        unsigned first=(op==42)?0:k, last=(op==42)?k:3;
        if (translate(c,aligned+first,last-first+1,&off)) {status=-2;break;}
        for (unsigned i=first;i<=last;i++) {
          unsigned src=(op==42)?(3-k+i):(i-k);
          c->ram[off+i-first]=(uint8_t)(b>>(8*src));
        }
      }
      break;
    case 40: case 41: case 43:
      addr=a+(uint32_t)imm;
      if (((op==41) && (addr&1u)) || (op==43 && (addr&3u)) ||
          translate(c,addr,op==43?4:(op==41?2:1),&off)) status=-2;
      else if(op==40) c->ram[off]=(uint8_t)b;
      else if(op==41) store16(c->ram+off,b);
      else store32(c->ram+off,b);
      break;
    default: status=-1;
  }
  if (!status && (op==1 || op==2 || op==3 || op==4 || op==5 || op==6 || op==7 ||
      op==20 || op==21 || op==22 || op==23 || (op==17 && rs==8) || (op==0 && (fn==8 || fn==9))))
    has_delay=1;
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
