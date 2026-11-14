#include "psp_ge_triangles.h"
#include "psp_platform_budget.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static void put16(unsigned char *p,unsigned v){p[0]=(uint8_t)v;p[1]=(uint8_t)(v>>8);}
static void vert(unsigned char *p,unsigned color,unsigned x,unsigned y,unsigned z){
 put16(p,color);put16(p+2,x);put16(p+4,y);put16(p+6,z);
}
static void valid(void){
 const uint32_t words[]={0x10080000u,0x01000020u,0x12800114u,
                         0x04030003u,0x0f000000u,0x0c000000u};
 unsigned char mem[48]={0};
 vert(mem+0,0x801fu,3,4,1);vert(mem+8,0x83e0u,100,70,2);
 vert(mem+16,0xfc00u,479,271,3);
 PspGeTriangleResult a;
 assert(psp_ge_triangles_decode(words,6,mem,sizeof mem,0x08000020u,&a)==0);
 assert(a.count==1&&a.consumed_commands==6);
 assert(a.triangles[0].v[0].color555==31);
 assert(a.triangles[0].v[1].x==100&&a.triangles[0].v[1].z==2);
 assert(a.triangles[0].v[2].y==271&&a.triangles[0].v[2].color555==0x7c00u);
 unsigned char double_tri[48];
 memcpy(double_tri,mem,24);memcpy(double_tri+24,mem,24);
 const uint32_t multi[]={0x10080000u,0x01000000u,0x12800114u,
                          0x04030006u,0x0f000000u,0x0c000000u};
 assert(psp_ge_triangles_decode(multi,6,double_tri,sizeof double_tri,
                                0x08000000u,&a)==0);
 assert(a.count==2&&a.triangles[1].v[2].x==479);
 /* Same shared GE command semantics on all DS, Pak, DSi and 3DS profiles. */
 for(int t=0;t<PSP_TARGET_COUNT;t++){
  PspPlatformBudget p;
  assert(psp_platform_budget_choose((PspTarget)t,&p)==0);
  assert(psp_platform_budget_validate(&p)==0);
  assert(psp_ge_triangles_decode(words,6,mem,sizeof mem,
                                 0x08000020u,&a)==0&&a.count==1);
 }
}
static void failure(void){
 uint32_t words[]={0x10080000u,0x01000000u,0x12800114u,
                   0x04030003u,0x0f000000u,0x0c000000u};
 unsigned char mem[24];
 vert(mem,0x801fu,3,4,1);vert(mem+8,0x83e0u,100,70,2);
 vert(mem+16,0xfc00u,479,271,3);
 PspGeTriangleResult out;
 #define CHECK_FAIL(expected) do { \
  memset(&out,0xa5,sizeof out); \
  assert(psp_ge_triangles_decode(words,6,mem,sizeof mem,0x08000000u,&out)==(expected)); \
  assert(out.count==0&&out.consumed_commands==0); \
 } while(0)
 words[2]=0x12800115u;CHECK_FAIL(PSP_GE_TRIANGLE_UNSUPPORTED);
 words[2]=0x12800114u;
 words[3]=0x04060002u;CHECK_FAIL(PSP_GE_TRIANGLE_UNSUPPORTED);
 words[3]=0x04030003u;
 words[1]=0x01000001u;CHECK_FAIL(PSP_GE_TRIANGLE_MALFORMED);
 words[1]=0x01000000u;
 words[4]=0x08000000u;CHECK_FAIL(PSP_GE_TRIANGLE_UNSUPPORTED);
 words[4]=0x0f000000u;
 words[5]=0x0c000001u;CHECK_FAIL(PSP_GE_TRIANGLE_MALFORMED);
 words[5]=0x0c000000u;
 mem[1]=0;CHECK_FAIL(PSP_GE_TRIANGLE_UNSUPPORTED);
 mem[1]=0x80;
 put16(mem+2,480);CHECK_FAIL(PSP_GE_TRIANGLE_MALFORMED);
 put16(mem+2,3);
 put16(mem+12,272);CHECK_FAIL(PSP_GE_TRIANGLE_MALFORMED);
 put16(mem+12,70);
 words[3]=0x04030063u;CHECK_FAIL(PSP_GE_TRIANGLE_OVERFLOW);
 words[3]=0x04030003u;
 memset(&out,0xa5,sizeof out);
 assert(psp_ge_triangles_decode(words,6,mem,23,0x08000000u,&out)==PSP_GE_TRIANGLE_MALFORMED);
 assert(out.count==0);
 assert(psp_ge_triangles_decode(words,6,mem,sizeof mem,0x08000001u,&out)==
        PSP_GE_TRIANGLE_MALFORMED);
 assert(psp_ge_triangles_decode(words,0,mem,sizeof mem,0,&out)==
        PSP_GE_TRIANGLE_BAD_ARGUMENT);
 assert(psp_ge_triangles_decode(words,6,NULL,0,0,&out)==
        PSP_GE_TRIANGLE_BAD_ARGUMENT);
 assert(psp_ge_triangles_decode(words,6,mem,sizeof mem,0,NULL)==
        PSP_GE_TRIANGLE_BAD_ARGUMENT);
 uint32_t huge[PSP_GE_TRIANGLE_MAX_COMMANDS+1]={0};
 assert(psp_ge_triangles_decode(huge,PSP_GE_TRIANGLE_MAX_COMMANDS+1,
                                mem,sizeof mem,0,&out)==PSP_GE_TRIANGLE_BAD_ARGUMENT);
 #undef CHECK_FAIL
}
int main(void){
 valid();failure();
 puts("PASS: strict PSP GE triangles across DS, DS+Pak, DSi, old/New 3DS");
 return 0;
}
