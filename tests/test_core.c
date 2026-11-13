#include "psp_core.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static void w32(unsigned char *p,unsigned x){p[0]=x;p[1]=x>>8;p[2]=x>>16;p[3]=x>>24;}
static void test_headers(void){
 unsigned char b[128]={0}; PspFileInfo i;
 memcpy(b,"\0PBP",4);w32(b+4,0x10000);for(int j=0;j<8;j++)w32(b+8+j*4,40+j*8);
 assert(psp_inspect(b,sizeof b,&i)==0&&i.type==PSP_FILE_PBP);
 w32(b+8+4*4,10);assert(psp_inspect(b,sizeof b,&i)==-2);
 memset(b,0,sizeof b);memcpy(b,"\177ELF",4);b[4]=1;b[5]=1;b[18]=8;w32(b+24,0x8800000);
 assert(psp_inspect(b,sizeof b,&i)==0&&i.type==PSP_FILE_ELF&&i.entry==0x8800000);
 b[18]=40;assert(psp_inspect(b,sizeof b,&i)==-2);
 assert(psp_inspect(NULL,10,&i)==-1);
}
static void test_cpu(void){
 unsigned char mem[256]={0};PspCpu c;unsigned base=0x08800000;
 w32(mem+0,0x24080007);w32(mem+4,0x24090005);w32(mem+8,0x01095021);
 w32(mem+12,0xac4a0040);w32(mem+16,0x8c4b0040);
 psp_cpu_init(&c,mem,sizeof mem,base,base);c.gpr[2]=base;
 for(int j=0;j<5;j++)assert(psp_cpu_step(&c)==0);
 assert(c.gpr[10]==12&&c.gpr[11]==12&&c.gpr[0]==0);
 memset(mem,0,sizeof mem);w32(mem,0x10000002);w32(mem+4,0x25080001);w32(mem+8,0x25080064);w32(mem+12,0x25080002);
 psp_cpu_init(&c,mem,sizeof mem,base,base);for(int j=0;j<3;j++)assert(psp_cpu_step(&c)==0);
 assert(c.gpr[8]==3);
 psp_cpu_init(&c,mem,sizeof mem,base,base+255);assert(psp_cpu_step(&c)==-2);
}
int main(void){test_headers();test_cpu();puts("PASS: PSP header validation and MIPS interpreter tests");return 0;}
