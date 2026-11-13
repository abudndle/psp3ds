#ifndef PSP_GUEST_WINDOW_H
#define PSP_GUEST_WINDOW_H
#include <stddef.h>
#include <stdint.h>
/* Volatile 16-bit bus window for a SDK-verified, single-bank Slot-2 RAM
 * peripheral. It is a checked DATA access adapter, not direct host RAM,
 * executable PSP CPU mapping, heap allocator or firmware memory model.
 * Caller must keep the peripheral open throughout use. */
typedef struct {
 volatile uint16_t *words;
 uint32_t guest_base;
 uint32_t capacity;
 int enabled;
} PspGuestWindow;
int psp_guest_window_init(PspGuestWindow *window,volatile uint16_t *words,
                           uint32_t guest_base,uint32_t capacity,
                           int verified,int one_bank);
int psp_guest_window_read8(const PspGuestWindow *w,uint32_t addr,uint8_t *out);
int psp_guest_window_read16(const PspGuestWindow *w,uint32_t addr,uint16_t *out);
int psp_guest_window_read32(const PspGuestWindow *w,uint32_t addr,uint32_t *out);
int psp_guest_window_write8(const PspGuestWindow *w,uint32_t addr,uint8_t value);
int psp_guest_window_write16(const PspGuestWindow *w,uint32_t addr,uint16_t value);
int psp_guest_window_write32(const PspGuestWindow *w,uint32_t addr,uint32_t value);
#endif
