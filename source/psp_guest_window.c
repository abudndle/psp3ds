#include "psp_guest_window.h"
#include <string.h>
static int offset(const PspGuestWindow *w,uint32_t addr,unsigned bytes,size_t *out){
 if(!w || !w->enabled || !w->words || !bytes || addr<w->guest_base)return -1;
 uint64_t pos=(uint64_t)addr-w->guest_base;
 if(pos>w->capacity || bytes>w->capacity-pos)return -1;
 *out=(size_t)pos;
 return 0;
}
int psp_guest_window_init(PspGuestWindow *w,volatile uint16_t *mem,
                          uint32_t base,uint32_t capacity,
                          int verified,int one_bank) {
 if(!w)return -1;
 memset(w,0,sizeof(*w));
 if(!verified || !one_bank || !mem || !capacity ||
    (base&3u) || (capacity&3u) || capacity>32u*1024u*1024u ||
    (uint64_t)base+capacity>UINT32_MAX+UINT64_C(1))return -1;
 w->words=mem;w->guest_base=base;w->capacity=capacity;w->enabled=1;
 return 0;
}
int psp_guest_window_read16(const PspGuestWindow *w,uint32_t addr,uint16_t *out){
 size_t off;
 if(!out || (addr&1u) || offset(w,addr,2,&off))return -1;
 *out=w->words[off/2];return 0;
}
int psp_guest_window_write16(const PspGuestWindow *w,uint32_t addr,uint16_t v){
 size_t off;
 if((addr&1u) || offset(w,addr,2,&off))return -1;
 w->words[off/2]=v;return 0;
}
int psp_guest_window_read8(const PspGuestWindow *w,uint32_t addr,uint8_t *out){
 size_t off;
 if(!out || offset(w,addr,1,&off))return -1;
 uint16_t word=w->words[off/2];
 *out=(uint8_t)((off&1u)?(word>>8):word);
 return 0;
}
int psp_guest_window_write8(const PspGuestWindow *w,uint32_t addr,uint8_t v){
 size_t off;
 if(offset(w,addr,1,&off))return -1;
 volatile uint16_t *word=w->words+off/2;
 uint16_t old=*word;
 *word=(uint16_t)((off&1u)?((old&0x00ffu)|((uint16_t)v<<8)):((old&0xff00u)|v));
 return 0;
}
int psp_guest_window_read32(const PspGuestWindow *w,uint32_t addr,uint32_t *out){
 size_t off;
 if(!out || (addr&3u) || offset(w,addr,4,&off))return -1;
 uint32_t low=w->words[off/2],high=w->words[off/2+1];
 *out=low|(high<<16);return 0;
}
int psp_guest_window_write32(const PspGuestWindow *w,uint32_t addr,uint32_t v){
 size_t off;
 if((addr&3u) || offset(w,addr,4,&off))return -1;
 w->words[off/2]=(uint16_t)v;
 w->words[off/2+1]=(uint16_t)(v>>16);
 return 0;
}
