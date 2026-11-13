#ifndef PSP_GUEST_BUS_H
#define PSP_GUEST_BUS_H
#include "psp_guest_window.h"
#include <stdint.h>
#include <stddef.h>
/* Explicit, nonoverlapping guest DATA regions.
 * Hot region is native internal RAM; cold region uses 16-bit Slot-2 access.
 * This is not wired to psp_cpu_step() or general PSP executable mapping.
 * The external peripheral must stay open while this bus is active. */
typedef struct {
 uint8_t *hot;
 uint32_t hot_base;
 size_t hot_size;
 PspGuestWindow cold;
 int has_cold;
} PspGuestBus;
int psp_guest_bus_init(PspGuestBus *bus,uint8_t *hot,uint32_t hot_base,
                       size_t hot_size,const PspGuestWindow *cold);
int psp_guest_bus_read8(const PspGuestBus *bus,uint32_t addr,uint8_t *value);
int psp_guest_bus_read16(const PspGuestBus *bus,uint32_t addr,uint16_t *value);
int psp_guest_bus_read32(const PspGuestBus *bus,uint32_t addr,uint32_t *value);
int psp_guest_bus_write8(const PspGuestBus *bus,uint32_t addr,uint8_t value);
int psp_guest_bus_write16(const PspGuestBus *bus,uint32_t addr,uint16_t value);
int psp_guest_bus_write32(const PspGuestBus *bus,uint32_t addr,uint32_t value);
#endif
