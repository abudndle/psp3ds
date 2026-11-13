#include "psp_guest_window.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#define BASE UINT32_C(0x08800000)
int main(void){
 uint16_t ram[128],saved[128];
 for(unsigned i=0;i<128;i++)ram[i]=(uint16_t)(0x5100u^i);
 memcpy(saved,ram,sizeof ram);
 PspGuestWindow w;
 assert(psp_guest_window_init(&w,ram,BASE,sizeof ram,1,1)==0);
 uint8_t b;uint16_t h;uint32_t word;
 assert(psp_guest_window_write32(&w,BASE+4,0x12345678u)==0);
 assert(psp_guest_window_read32(&w,BASE+4,&word)==0&&word==0x12345678u);
 assert(psp_guest_window_read16(&w,BASE+4,&h)==0&&h==0x5678u);
 assert(psp_guest_window_read16(&w,BASE+6,&h)==0&&h==0x1234u);
 assert(psp_guest_window_write8(&w,BASE+5,0xaau)==0);
 assert(psp_guest_window_read8(&w,BASE+5,&b)==0&&b==0xaau);
 assert(psp_guest_window_read16(&w,BASE+4,&h)==0&&h==0xaa78u);
 assert(psp_guest_window_write16(&w,BASE+sizeof ram-2,0xdead)==0);
 assert(psp_guest_window_read16(&w,BASE+sizeof ram-2,&h)==0&&h==0xdead);
 assert(psp_guest_window_read32(&w,BASE+sizeof ram-4,&word)==0);
 assert(psp_guest_window_write32(&w,BASE+sizeof ram-3,0)==-1);
 assert(psp_guest_window_read16(&w,BASE+1,&h)==-1);
 assert(psp_guest_window_read32(&w,BASE+2,&word)==-1);
 assert(psp_guest_window_write8(&w,BASE-1,5)==-1);
 assert(psp_guest_window_write8(&w,BASE+sizeof ram,5)==-1);
 assert(psp_guest_window_write32(&w,UINT32_MAX-3u,5)==-1);
 assert(psp_guest_window_read8(&w,BASE,NULL)==-1);
 memcpy(ram,saved,sizeof ram);
 assert(memcmp(ram,saved,sizeof ram)==0);
 assert(psp_guest_window_init(&w,ram,BASE,sizeof ram,0,1)==-1);
 assert(psp_guest_window_write16(&w,BASE,7)==-1);
 assert(psp_guest_window_init(&w,ram,BASE,sizeof ram,1,0)==-1);
 assert(psp_guest_window_init(&w,ram,BASE+1,sizeof ram,1,1)==-1);
 assert(psp_guest_window_init(&w,ram,0xfffffff0u,sizeof ram,1,1)==-1);
 puts("PASS: checked 8/16/32-bit volatile guest memory window and fail-closed mapping");
 return 0;
}
