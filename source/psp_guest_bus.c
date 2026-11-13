#include "psp_guest_bus.h"
#include <stdint.h>
#include <string.h>
static int hot_offset(const PspGuestBus *bus,uint32_t addr,unsigned bytes,size_t *out) {
 if(!bus||!bus->hot||addr<bus->hot_base)return -1;
 uint64_t at=(uint64_t)addr-bus->hot_base;
 if(at>bus->hot_size||bytes>bus->hot_size-at)return -1;
 *out=(size_t)at;return 0;
}
static int in_cold(const PspGuestBus *bus,uint32_t addr,unsigned bytes){
 if(!bus||!bus->has_cold||!bus->cold.enabled||addr<bus->cold.guest_base)return 0;
 uint64_t at=(uint64_t)addr-bus->cold.guest_base;
 return at<=bus->cold.capacity && bytes<=bus->cold.capacity-at;
}
int psp_guest_bus_init(PspGuestBus *bus,uint8_t *hot,uint32_t base,
                       size_t bytes,const PspGuestWindow *cold){
 if(!bus)return -1;
 memset(bus,0,sizeof(*bus));
 if(!hot||!bytes||bytes>32u*1024u*1024u ||
    (uint64_t)base+bytes>UINT32_MAX+UINT64_C(1))return -1;
 if(cold){
  if(!cold->enabled||!cold->words||!cold->capacity||
     (uint64_t)cold->guest_base+cold->capacity>UINT32_MAX+UINT64_C(1))return -1;
  uint64_t first=base,last=first+bytes;
  uint64_t ext=cold->guest_base,ext_last=ext+cold->capacity;
  if(first<ext_last && ext<last)return -1;
  bus->cold=*cold;bus->has_cold=1;
 }
 bus->hot=hot;bus->hot_base=base;bus->hot_size=bytes;
 return 0;
}
int psp_guest_bus_read8(const PspGuestBus *bus,uint32_t addr,uint8_t *value){
 size_t at;
 if(!value)return -1;
 if(!hot_offset(bus,addr,1,&at)){*value=bus->hot[at];return 0;}
 if(in_cold(bus,addr,1))return psp_guest_window_read8(&bus->cold,addr,value);
 return -1;
}
int psp_guest_bus_read16(const PspGuestBus *bus,uint32_t addr,uint16_t *value){
 size_t at;
 if(!value||(addr&1u))return -1;
 if(!hot_offset(bus,addr,2,&at)){
  *value=(uint16_t)(bus->hot[at]|(uint16_t)bus->hot[at+1]<<8);return 0;
 }
 if(in_cold(bus,addr,2))return psp_guest_window_read16(&bus->cold,addr,value);
 return -1;
}
int psp_guest_bus_read32(const PspGuestBus *bus,uint32_t addr,uint32_t *value){
 size_t at;
 if(!value||(addr&3u))return -1;
 if(!hot_offset(bus,addr,4,&at)){
  *value=(uint32_t)bus->hot[at]|(uint32_t)bus->hot[at+1]<<8|
         (uint32_t)bus->hot[at+2]<<16|(uint32_t)bus->hot[at+3]<<24;
  return 0;
 }
 if(in_cold(bus,addr,4))return psp_guest_window_read32(&bus->cold,addr,value);
 return -1;
}
int psp_guest_bus_write8(const PspGuestBus *bus,uint32_t addr,uint8_t value){
 size_t at;
 if(!hot_offset(bus,addr,1,&at)){bus->hot[at]=value;return 0;}
 if(in_cold(bus,addr,1))return psp_guest_window_write8(&bus->cold,addr,value);
 return -1;
}
int psp_guest_bus_write16(const PspGuestBus *bus,uint32_t addr,uint16_t value){
 size_t at;
 if(addr&1u)return -1;
 if(!hot_offset(bus,addr,2,&at)){
  bus->hot[at]=(uint8_t)value;bus->hot[at+1]=(uint8_t)(value>>8);return 0;
 }
 if(in_cold(bus,addr,2))return psp_guest_window_write16(&bus->cold,addr,value);
 return -1;
}
int psp_guest_bus_write32(const PspGuestBus *bus,uint32_t addr,uint32_t value){
 size_t at;
 if(addr&3u)return -1;
 if(!hot_offset(bus,addr,4,&at)){
  for(unsigned i=0;i<4;i++)bus->hot[at+i]=(uint8_t)(value>>(8*i));
  return 0;
 }
 if(in_cold(bus,addr,4))return psp_guest_window_write32(&bus->cold,addr,value);
 return -1;
}
