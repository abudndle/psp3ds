#include "psp_io.h"
#include "psp_hle.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define S 2048u
#define BASE 0x08800000u
static void put32(uint8_t *p,uint32_t v){for(unsigned i=0;i<4;i++)p[i]=(uint8_t)(v>>(8*i));}
static void make_iso(const char *name,char marker){
 FILE *f=fopen(name,"wb");assert(f);uint8_t b[S];
 for(unsigned lba=0;lba<20;lba++){
  memset(b,0,sizeof b);
  if(lba==16){
   b[0]=1;memcpy(b+1,"CD001",5);b[6]=1;
   b[156]=34;put32(b+158,18);put32(b+166,S);
   b[181]=2;b[184]=1;b[188]=1;
  }
  if(lba==18){
   b[0]=44;put32(b+2,19);put32(b+10,10);
   b[28]=1;b[31]=1;b[32]=10;memcpy(b+33,"DATA.BIN;1",10);
  }
  if(lba==19){memcpy(b,"123456789",9);b[9]=(uint8_t)marker;}
  assert(fwrite(b,1,S,f)==S);
 }
 assert(fclose(f)==0);
}
static void syscall(PspHle *hle,PspCpu *cpu,uint32_t id){
 put32(cpu->ram,(id<<6)|12u);
 cpu->pc=BASE;cpu->next_pc=BASE+4;cpu->delay_slot=0;
 assert(psp_hle_step(hle,cpu)==1);
}
int main(void){
 const char *a="test_io_a.iso",*b="test_io_b.iso";
 make_iso(a,'A');make_iso(b,'B');
 PspUmd *umd=malloc(sizeof(*umd));assert(umd);
 psp_umd_init(umd);assert(psp_umd_mount(umd,a)==0);
 PspIo io;psp_io_init(&io,umd);
 assert(psp_io_open(&io,"disc0:/DATA.BIN",PSP_IO_RDONLY)==3);
 assert(psp_io_open(&io,"disc0:/DATA.BIN",PSP_IO_RDONLY)==4);
 assert(psp_io_open(&io,"disc0:/DATA.BIN",2)==-1);
 assert(psp_io_open(&io,"ms0:/DATA.BIN",1)==-1);
 assert(psp_io_open(&io,"disc0:/../DATA.BIN",1)==-1);
 char buffer[20]={0};
 assert(psp_io_read(&io,3,buffer,4)==4 && !memcmp(buffer,"1234",4));
 assert(psp_io_read(&io,4,buffer,3)==3 && !memcmp(buffer,"123",3));
 assert(psp_io_seek32(&io,3,-2,1)==2);
 assert(psp_io_seek64(&io,3,INT64_MAX,1)==-1);
 assert(psp_io_seek64(&io,3,INT64_MIN,1)==-1);
 assert(psp_io_seek64(&io,3,0,1)==2);
 assert(psp_io_read(&io,3,buffer,15)==8 && !memcmp(buffer,"3456789A",8));
 assert(psp_io_read(&io,3,buffer,1)==0);
 assert(psp_io_seek32(&io,3,-1,2)==9);
 assert(psp_io_read(&io,3,buffer,1)==1 && buffer[0]=='A');
 assert(psp_io_seek32(&io,3,-1,0)==-1);
 assert(psp_io_seek32(&io,3,1,3)==-1);
 assert(psp_io_close(&io,3)==0 && psp_io_read(&io,3,buffer,1)==-1);
 PspHle h;psp_hle_init(&h);psp_hle_attach_io(&h,&io);
 assert(psp_hle_bind(&h,101,PSP_HLE_IO_OPEN)==0);
 assert(psp_hle_bind(&h,102,PSP_HLE_IO_READ)==0);
 assert(psp_hle_bind(&h,103,PSP_HLE_IO_LSEEK32)==0);
 assert(psp_hle_bind(&h,104,PSP_HLE_IO_CLOSE)==0);
 uint8_t ram[512]={0};PspCpu cpu;psp_cpu_init(&cpu,ram,sizeof ram,BASE,BASE);
 memcpy(ram+80,"disc0:/DATA.BIN",16);
 cpu.gpr[4]=BASE+80;cpu.gpr[5]=PSP_IO_RDONLY;
 syscall(&h,&cpu,101);int32_t fd=(int32_t)cpu.gpr[2];assert(fd==3);
 cpu.gpr[4]=(uint32_t)fd;cpu.gpr[5]=BASE+120;cpu.gpr[6]=4;
 syscall(&h,&cpu,102);assert(cpu.gpr[2]==4 && !memcmp(ram+120,"1234",4));
 cpu.gpr[4]=(uint32_t)fd;cpu.gpr[5]=BASE+510;cpu.gpr[6]=4;
 syscall(&h,&cpu,102);assert(cpu.gpr[2]==UINT32_MAX);
 cpu.gpr[4]=(uint32_t)fd;cpu.gpr[5]=BASE+120;cpu.gpr[6]=3;
 syscall(&h,&cpu,102);assert(cpu.gpr[2]==3 && !memcmp(ram+120,"567",3));
 cpu.gpr[4]=(uint32_t)fd;cpu.gpr[5]=(uint32_t)-1;cpu.gpr[6]=2;
 syscall(&h,&cpu,103);assert(cpu.gpr[2]==9);
 cpu.gpr[4]=(uint32_t)fd;cpu.gpr[5]=BASE+120;cpu.gpr[6]=8;
 syscall(&h,&cpu,102);assert(cpu.gpr[2]==1 && ram[120]=='A');
 cpu.gpr[4]=(uint32_t)fd;syscall(&h,&cpu,104);assert(cpu.gpr[2]==0);
 syscall(&h,&cpu,104);assert(cpu.gpr[2]==UINT32_MAX);
 int32_t old=psp_io_open(&io,"disc0:/DATA.BIN",1);assert(old==3);
 assert(psp_umd_mount(umd,b)==0);
 assert(psp_io_read(&io,old,buffer,1)==-1);
 assert(psp_io_open(&io,"disc0:/DATA.BIN",1)==3);
 assert(psp_io_seek32(&io,3,9,0)==9);
 assert(psp_io_read(&io,3,buffer,1)==1 && buffer[0]=='B');
 psp_umd_unmount(umd);free(umd);
 assert(remove(a)==0&&remove(b)==0);
 puts("PASS: read-only PSP HLE file handles, bounds, seek, EOF and disc-swap isolation");
 return 0;
}
