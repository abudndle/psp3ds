#ifndef PSP_SLOT2_PROBE_H
#define PSP_SLOT2_PROBE_H
#include <stddef.h>
#include <stdint.h>

/* Non-destructive probe of SDK-confirmed Slot-2 RAM.
 * Caller MUST establish that the peripheral is present, opened for RAM
 * access, has one flat bank, and supports 16-bit reads/writes.
 * Do not call this on arbitrary GBA Slot-2 mapped ROM or absent hardware.
 * Tests at most two locations, restores them immediately and never adds
 * Slot-2 memory to the normal C heap. */
typedef enum {
 PSP_SLOT2_UNAVAILABLE=0,
 PSP_SLOT2_UNVERIFIED=1,
 PSP_SLOT2_RW_VERIFIED=2
} PspSlot2Status;
typedef struct {
 PspSlot2Status status;
 uint32_t detected_bytes;
 uint32_t probed_locations;
} PspSlot2Result;
PspSlot2Result psp_slot2_probe(volatile uint16_t *base, uint32_t bytes,
                               int peripheral_ram_confirmed, int single_bank);
/* A bounded sampled memory sweep across the SDK-reported flat Slot-2 bank.
 * At most 64 evenly spaced 16-bit locations are tested; their previous
 * contents are restored immediately. The result proves only the sampled
 * addresses, not contiguous availability, lack of aliasing, or suitability
 * for guest PSP allocation. Call only after peripheralSlot2Open(). */
#define PSP_SLOT2_MAX_SAMPLES 64u
PspSlot2Result psp_slot2_probe_samples(volatile uint16_t *base,
                                      uint32_t bytes,
                                      int peripheral_ram_confirmed,
                                      int single_bank,
                                      unsigned samples);
#endif
