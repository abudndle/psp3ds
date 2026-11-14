#ifndef PSP_SLOT2_BUS_H
#define PSP_SLOT2_BUS_H
#include "psp_guest_bus.h"
#include "psp_slot2_budget.h"
/* A trusted hardware adapter must open and verify the peripheral first.
 * This function only binds a verified flat RAM window to the existing
 * guest DATA bus; it does not probe, allocate, or grant executable access.
 * Returns 1 with cold memory, 0 with safe hot-only fallback, -1 on error. */
int psp_slot2_bus_init(PspGuestBus *bus, uint8_t *hot,
                       uint32_t hot_base, size_t hot_size,
                       const PspSlot2Result *probe,
                       volatile uint16_t *confirmed_ram,
                       uint32_t cold_guest_base,
                       PspPlatformBudget *selected);
#endif
