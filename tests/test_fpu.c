#include "psp_core.h"
#include <assert.h>
#include <fenv.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#define BASE 0x08800000u
static void w32(unsigned char *p, unsigned x) { p[0]=x; p[1]=x>>8; p[2]=x>>16; p[3]=x>>24; }
static unsigned fb(float f) { unsigned v; memcpy(&v,&f,4); return v; }
static float bf(unsigned v) { float f; memcpy(&f,&v,4); return f; }
/* COP1 encodings */
static unsigned cop1(unsigned fmt, unsigned ft, unsigned fs, unsigned fd, unsigned fn) {
  return 0x44000000u | fmt<<21 | ft<<16 | fs<<11 | fd<<6 | fn;
}
static unsigned MTC1(unsigned rt, unsigned fs) { return cop1(4,rt,fs,0,0); }
static unsigned MFC1(unsigned rt, unsigned fs) { return cop1(0,rt,fs,0,0); }
static unsigned CTC1(unsigned rt) { return cop1(6,rt,31,0,0); }
static unsigned CFC1(unsigned rt) { return cop1(2,rt,31,0,0); }
static unsigned S(unsigned ft, unsigned fs, unsigned fd, unsigned fn) { return cop1(16,ft,fs,fd,fn); }

static unsigned char mem[1024];
static PspCpu cpu;
static void load(const unsigned *prog, int n) {
  memset(mem,0,sizeof mem);
  for (int i=0;i<n;i++) w32(mem+4*i,prog[i]);
  psp_cpu_init(&cpu,mem,sizeof mem,BASE,BASE);
}
static void run(int n) { for (int i=0;i<n;i++) assert(psp_cpu_step(&cpu)==0); }

static const int host_rm[4] = { FE_TONEAREST, FE_TOWARDZERO, FE_UPWARD, FE_DOWNWARD };

static unsigned run_op(unsigned fn, unsigned a, unsigned b, unsigned rm) {
  unsigned p[] = { MTC1(8,0), MTC1(9,1), CTC1(10), S(1,0,2,fn), MFC1(11,2) };
  load(p,5); cpu.gpr[8]=a; cpu.gpr[9]=b; cpu.gpr[10]=rm; run(5);
  return cpu.gpr[11];
}
static unsigned next_rand(unsigned *x) { *x^=*x<<13; *x^=*x>>17; *x^=*x<<5; return *x; }

/* Compare directed-rounding arithmetic against the host FPU under every rounding mode. */
static void test_rounding_vs_host(void) {
  unsigned x=0x2545F491u;
  for (int i=0;i<60000;i++) {
    unsigned a=next_rand(&x), b=next_rand(&x);
    if (i%4==0) { b=(a&0x807fffffu)|((b&0x0300000u)<<0); }          /* close magnitudes */
    if (i%7==0) { a&=0x80ffffffu; b&=0x80ffffffu; }                  /* tiny/denormal range */
    if (((a>>23)&255)==255 || ((b>>23)&255)==255) continue;           /* no inf/NaN here */
    for (unsigned rm=0; rm<4; rm++) {
      volatile float va=bf(a), vb=bf(b), r;
      volatile int vi=(int)a;
      unsigned w_add, w_sub, w_mul, w_div=0, w_sqrt=0, w_cvt;
      /* Expected values come from the host FPU in the directed mode... */
      fesetround(host_rm[rm]);
      r=va+vb; w_add=fb(r);
      r=va-vb; w_sub=fb(r);
      r=va*vb; w_mul=fb(r);
      if ((b&0x7fffffffu)!=0) { r=va/vb; w_div=fb(r); }
      if (!(a>>31)) { r=sqrtf(va); w_sqrt=fb(r); }
      r=(float)vi; w_cvt=fb(r);
      /* ...while the emulator itself runs under the default host mode, so its own
         correction logic (not the host's rounding) is what gets tested. */
      fesetround(FE_TONEAREST);
      unsigned got;
      got=run_op(0,a,b,rm); if (got!=w_add) { printf("add %08x %08x rm%u got %08x want %08x\n",a,b,rm,got,w_add); assert(0); }
      got=run_op(1,a,b,rm); if (got!=w_sub) { printf("sub %08x %08x rm%u got %08x want %08x\n",a,b,rm,got,w_sub); assert(0); }
      got=run_op(2,a,b,rm); if (got!=w_mul) { printf("mul %08x %08x rm%u got %08x want %08x\n",a,b,rm,got,w_mul); assert(0); }
      if ((b&0x7fffffffu)!=0) { got=run_op(3,a,b,rm); if (got!=w_div) { printf("div %08x %08x rm%u got %08x want %08x\n",a,b,rm,got,w_div); assert(0); } }
      if (!(a>>31)) { got=run_op(4,a,a,rm); if (got!=w_sqrt) { printf("sqrt %08x rm%u got %08x want %08x\n",a,rm,got,w_sqrt); assert(0); } }
      { unsigned q[] = { MTC1(8,0), CTC1(9), cop1(20,0,0,1,32), MFC1(10,1) };
        load(q,4); cpu.gpr[8]=a; cpu.gpr[9]=rm; run(4); got=cpu.gpr[10];
        if (got!=w_cvt) { printf("cvt.s.w %08x rm%u got %08x want %08x\n",a,rm,got,w_cvt); assert(0); } }
    }
  }
}

/* NaNs the FPU makes from scratch are the positive quiet NaN; inputs' NaNs propagate. */
static void test_nan_and_fir(void) {
  static const struct { unsigned a, b, fn, want; } t[] = {
    {0x00000000,0x00000000,3,0x7fc00000},  /* 0/0 */
    {0x7f800000,0x7f800000,1,0x7fc00000},  /* inf-inf */
    {0xff800000,0x7f800000,0,0x7fc00000},  /* -inf+inf */
    {0x7f800000,0x00000000,2,0x7fc00000},  /* inf*0 */
    {0xff800000,0x7f800000,3,0x7fc00000},  /* -inf/inf: positive NaN */
    {0x00000000,0xff800000,3,0x80000000},  /* 0/-inf = -0 */
    {0x80000000,0x00000000,1,0x80000000},  /* -0 - 0 = -0 */
    {0x7fc00000,0x3f800000,0,0x7fc00000},  /* NaN propagates */
  };
  for (unsigned i=0;i<sizeof t/sizeof *t;i++) assert(run_op(t[i].fn,t[i].a,t[i].b,0)==t[i].want);
  unsigned p[] = { cop1(2,8,0,0,0), cop1(2,9,25,0,0), cop1(2,10,28,0,0), cop1(2,11,3,0,0) };
  load(p,4); run(3);
  assert(cpu.gpr[8]==0x00003351u && cpu.gpr[9]==0 && cpu.gpr[10]==0);
  assert(psp_cpu_step(&cpu)==-1);                                             /* other FCRs fail closed */
}

static void test_arith(void) {
  unsigned p[] = { MTC1(8,0), MTC1(9,1),
    S(1,0,2,0), S(1,0,3,1), S(1,0,4,2), S(1,0,5,3), S(0,0,6,5), S(0,0,7,7), S(0,0,8,6),
    MFC1(10,2), MFC1(11,3), MFC1(12,4), MFC1(13,5), MFC1(14,6), MFC1(15,7), MFC1(16,8) };
  load(p,(int)(sizeof p/sizeof *p));
  cpu.gpr[8]=fb(-3.0f); cpu.gpr[9]=fb(2.0f);
  run((int)(sizeof p/sizeof *p));
  assert(cpu.gpr[10]==fb(-1.0f));   /* add */
  assert(cpu.gpr[11]==fb(-5.0f));   /* sub */
  assert(cpu.gpr[12]==fb(-6.0f));   /* mul */
  assert(cpu.gpr[13]==fb(-1.5f));   /* div */
  assert(cpu.gpr[14]==fb(3.0f));    /* abs */
  assert(cpu.gpr[15]==fb(3.0f));    /* neg */
  assert(cpu.gpr[16]==fb(-3.0f));   /* mov */
}

static void test_sqrt(void) {
  /* exhaustive-ish: powers of two, denormals, neighbours of 1, and a pseudo-random sweep */
  unsigned seeds[] = {0x00000001u,0x007fffffu,0x00800000u,0x3f7fffffu,0x3f800000u,0x3f800001u,
                      0x40000000u,0x40490fdbu,0x7f7fffffu,0x7f000000u,0x01000000u};
  unsigned x=0x12345678u;
  for (int i=0;i<200000;i++) {
    unsigned b;
    if (i<(int)(sizeof seeds/sizeof *seeds)) b=seeds[i];
    else { x^=x<<13; x^=x>>17; x^=x<<5; b=x&0x7fffffffu; if (b>=0x7f800000u) b&=0x7f7fffffu; }
    unsigned p[] = { MTC1(8,0), S(0,0,1,4), MFC1(9,1) };
    load(p,3); cpu.gpr[8]=b; run(3);
    float want=sqrtf(bf(b)); unsigned wb; memcpy(&wb,&want,4);
    if (cpu.gpr[9]!=wb) { printf("sqrt mismatch %08x got %08x want %08x\n",b,cpu.gpr[9],wb); assert(0); }
  }
  unsigned p[] = { MTC1(8,0), S(0,0,1,4), MFC1(9,1) };
  load(p,3); cpu.gpr[8]=0x80000000u; run(3); assert(cpu.gpr[9]==0x80000000u);         /* sqrt(-0)=-0 */
  load(p,3); cpu.gpr[8]=fb(-1.0f);   run(3); assert((cpu.gpr[9]&0x7fc00000u)==0x7fc00000u); /* NaN */
  load(p,3); cpu.gpr[8]=0x7f800000u; run(3); assert(cpu.gpr[9]==0x7f800000u);         /* +inf */
}

static void test_convert(void) {
  /* cvt.w.s honoring FCSR rounding mode, plus fixed-mode round/trunc/ceil/floor */
  struct { float v; int rm; int w; } cvt[] = {
    {2.5f,0,2},{3.5f,0,4},{-2.5f,0,-2},{-3.5f,0,-4},{2.4f,0,2},{2.6f,0,3},
    {2.9f,1,2},{-2.9f,1,-2},{2.1f,2,3},{-2.1f,2,-2},{2.9f,3,2},{-2.1f,3,-3},
    {3.0e9f,0,0x7fffffff},{-3.0e9f,0,(int)0x80000000},{2147483648.0f,1,0x7fffffff},{-2147483648.0f,1,(int)0x80000000},
    {8388609.0f,0,8388609},{0.0f,0,0}};
  for (unsigned i=0;i<sizeof cvt/sizeof *cvt;i++) {
    unsigned p[] = { MTC1(8,0), CTC1(9), S(0,0,1,36), MFC1(10,1), CFC1(11) };
    load(p,5); cpu.gpr[8]=fb(cvt[i].v); cpu.gpr[9]=(unsigned)cvt[i].rm; run(5);
    assert((int)cpu.gpr[10]==cvt[i].w);
    assert(cpu.gpr[11]==(unsigned)cvt[i].rm);
  }
  unsigned p[] = { MTC1(8,0), S(0,0,1,12), S(0,0,2,13), S(0,0,3,14), S(0,0,4,15),
                   MFC1(10,1), MFC1(11,2), MFC1(12,3), MFC1(13,4) };
  load(p,9); cpu.gpr[8]=fb(-2.5f); run(9);
  assert((int)cpu.gpr[10]==-2 && (int)cpu.gpr[11]==-2 && (int)cpu.gpr[12]==-2 && (int)cpu.gpr[13]==-3);
  unsigned q[] = { MTC1(8,0), S(0,0,1,36), MFC1(9,1) };
  load(q,3); cpu.gpr[8]=0x7fc00000u; run(3); assert(cpu.gpr[9]==0x7fffffffu);          /* NaN */
  unsigned r[] = { MTC1(8,0), cop1(20,0,0,1,32), MFC1(9,1) };                           /* cvt.s.w */
  load(r,3); cpu.gpr[8]=(unsigned)-7; run(3); assert(cpu.gpr[9]==fb(-7.0f));
  load(r,3); cpu.gpr[8]=16777217u; run(3); assert(cpu.gpr[9]==fb(16777216.0f));
  /* CTC1 keeps only defined FCSR bits */
  unsigned c[] = { CTC1(8), CFC1(9) };
  load(c,2); cpu.gpr[8]=0xffffffffu; run(2); assert(cpu.gpr[9]==0x0183ffffu);
}

static void test_compare_branch(void) {
  /* c.lt.s f0,f1 ; bc1t +2 ; addiu r12,r0,5 (delay) ; addiu r13,r0,99 (skipped) ; addiu r14,r0,7 */
  unsigned bc1t = 0x45010000u | 2u, bc1f = 0x45000000u | 2u, bc1tl = 0x45030000u | 2u, bc1fl = 0x45020000u | 2u;
  unsigned p[] = { MTC1(8,0), MTC1(9,1), S(1,0,0,60), bc1t, 0x240c0005u, 0x240d0063u, 0x240e0007u };
  load(p,7); cpu.gpr[8]=fb(1.0f); cpu.gpr[9]=fb(2.0f); run(6);
  assert(cpu.gpr[12]==5 && cpu.gpr[13]==0 && cpu.gpr[14]==7);
  load(p,7); cpu.gpr[8]=fb(3.0f); cpu.gpr[9]=fb(2.0f); run(7);               /* not taken */
  assert(cpu.gpr[12]==5 && cpu.gpr[13]==99 && cpu.gpr[14]==7);
  /* NaN: ordered compares are false, c.un and c.ueq are true */
  unsigned n1[] = { MTC1(8,0), MTC1(9,1), S(1,0,0,50), MFC1(0,0), CFC1(10), S(1,0,0,49), CFC1(11), S(1,0,0,51), CFC1(12) };
  load(n1,9); cpu.gpr[8]=0x7fc00000u; cpu.gpr[9]=fb(1.0f); run(9);
  assert(!(cpu.gpr[10]&0x800000u) && (cpu.gpr[11]&0x800000u) && (cpu.gpr[12]&0x800000u));
  /* c.le / c.eq on equals */
  unsigned e[] = { MTC1(8,0), MTC1(9,1), S(1,0,0,62), CFC1(10), S(1,0,0,50), CFC1(11), S(1,0,0,60), CFC1(12) };
  load(e,8); cpu.gpr[8]=fb(2.0f); cpu.gpr[9]=fb(2.0f); run(8);
  assert((cpu.gpr[10]&0x800000u) && (cpu.gpr[11]&0x800000u) && !(cpu.gpr[12]&0x800000u));
  /* bc1f taken when condition false (c.eq 1<2), delay slot executes */
  unsigned f[] = { MTC1(8,0), MTC1(9,1), S(1,0,0,50), bc1f, 0x240c0005u, 0x240d0063u, 0x240e0007u };
  load(f,7); cpu.gpr[8]=fb(1.0f); cpu.gpr[9]=fb(2.0f); run(6);
  assert(cpu.gpr[12]==5 && cpu.gpr[13]==0 && cpu.gpr[14]==7);
  /* branch-likely: annulled delay slot when not taken, executed when taken */
  unsigned l[] = { MTC1(8,0), MTC1(9,1), S(1,0,0,60), bc1tl, 0x240c0005u, 0x240d0063u, 0x240e0007u };
  load(l,7); cpu.gpr[8]=fb(3.0f); cpu.gpr[9]=fb(2.0f); run(6);                /* not taken: skip delay */
  assert(cpu.gpr[12]==0 && cpu.gpr[13]==99);
  load(l,7); cpu.gpr[8]=fb(1.0f); cpu.gpr[9]=fb(2.0f); run(6);                /* taken */
  assert(cpu.gpr[12]==5 && cpu.gpr[13]==0 && cpu.gpr[14]==7);
  unsigned fl[] = { MTC1(8,0), MTC1(9,1), S(1,0,0,60), bc1fl, 0x240c0005u, 0x240d0063u, 0x240e0007u };
  load(fl,7); cpu.gpr[8]=fb(1.0f); cpu.gpr[9]=fb(2.0f); run(5);               /* cond true: bc1fl not taken, annulled */
  assert(cpu.gpr[12]==0);
  unsigned bad[] = { 0x45040000u };                                            /* invalid BC1 variant */
  load(bad,1); assert(psp_cpu_step(&cpu)==-1);
}

static void test_memory_and_faults(void) {
  /* lwc1 f2,0x40(r2) ; add.s f3,f2,f2 ; swc1 f3,0x44(r2) ; lw r10,0x44(r2) */
  unsigned prog[] = { 0xc4420040u, S(2,2,3,0), 0xe4430044u, 0x8c4a0044u };
  load(prog,4); w32(mem+0x40,fb(1.25f)); cpu.gpr[2]=BASE; run(4);
  assert(cpu.gpr[10]==fb(2.5f) && cpu.fpr[2]==fb(1.25f));
  unsigned mis[] = { 0xc4420041u };                                           /* misaligned lwc1 */
  load(mis,1); cpu.gpr[2]=BASE; assert(psp_cpu_step(&cpu)==-2);
  unsigned oob[] = { 0xc4420000u };                                           /* out of RAM */
  load(oob,1); cpu.gpr[2]=BASE+0x10000u; assert(psp_cpu_step(&cpu)==-2);
  unsigned dbl[] = { cop1(17,0,0,0,0) };                                      /* double: unsupported, fail closed */
  load(dbl,1); assert(psp_cpu_step(&cpu)==-1);
  unsigned mov0[] = { MTC1(8,0), MFC1(0,0) };                                 /* writes to r0 are ignored */
  load(mov0,2); cpu.gpr[8]=123; run(2); assert(cpu.gpr[0]==0);
}

int main(void) {
  test_arith(); test_nan_and_fir(); test_rounding_vs_host(); test_sqrt(); test_convert(); test_compare_branch(); test_memory_and_faults();
  puts("PASS: PSP COP1 single-precision FPU tests");
  return 0;
}
