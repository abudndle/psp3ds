#ifndef PSP_NINTENDO_INPUT_H
#define PSP_NINTENDO_INPUT_H

#include <stdint.h>
#include "psp_platform_budget.h"

#ifdef __cplusplus
extern "C" {
#endif

/* These native key positions are shared by libctru and libnds for
 * A/B/SELECT/START/D-pad/R/L/X/Y (bits 0 through 11).
 * Pass physical D-pad bits only: 3DS circle-pad direction aliases live
 * in the high bits and must not leak into PSP D-pad buttons. */
#define PSP_NINTENDO_KEY_A       (1u << 0)
#define PSP_NINTENDO_KEY_B       (1u << 1)
#define PSP_NINTENDO_KEY_SELECT  (1u << 2)
#define PSP_NINTENDO_KEY_START   (1u << 3)
#define PSP_NINTENDO_KEY_RIGHT   (1u << 4)
#define PSP_NINTENDO_KEY_LEFT    (1u << 5)
#define PSP_NINTENDO_KEY_UP      (1u << 6)
#define PSP_NINTENDO_KEY_DOWN    (1u << 7)
#define PSP_NINTENDO_KEY_R      (1u << 8)
#define PSP_NINTENDO_KEY_L      (1u << 9)
#define PSP_NINTENDO_KEY_X      (1u << 10)
#define PSP_NINTENDO_KEY_Y      (1u << 11)
#define PSP_NINTENDO_KEY_MASK   0x0fffu

/* Upstream PPSSPP's sceCtrl.h-compatible values. */
#define PSP_CTRL_SELECT      0x0001u
#define PSP_CTRL_START       0x0008u
#define PSP_CTRL_UP          0x0010u
#define PSP_CTRL_RIGHT       0x0020u
#define PSP_CTRL_DOWN        0x0040u
#define PSP_CTRL_LEFT        0x0080u
#define PSP_CTRL_LTRIGGER    0x0100u
#define PSP_CTRL_RTRIGGER    0x0200u
#define PSP_CTRL_TRIANGLE    0x1000u
#define PSP_CTRL_CIRCLE      0x2000u
#define PSP_CTRL_CROSS       0x4000u
#define PSP_CTRL_SQUARE      0x8000u

typedef struct {
 uint32_t buttons; /* PSP sceCtrl-style bitmask */
 uint8_t lx, ly;   /* PSP analog: 128 centered, 0..255 */
} PspNintendoCtrl;

/* No floating point, heap or platform graphics/audio dependencies.
 * 3DS: map actual libctru circlePosition dx/dy (nominal +/-156).
 * DS/DS Lite, DS+Slot2 and DSi: synthesize PSP analog using D-pad.
 * This enables an analog-only game to receive input even on DS, but
 * does not claim hardware or emulated PSP-game compatibility.
 * 3DS circle-pad direction alias key bits are ignored. */
int psp_nintendo_ctrl_sample(PspTarget target, uint32_t native_keys,
                             int32_t circle_dx, int32_t circle_dy,
                             PspNintendoCtrl *out);

#ifdef __cplusplus
}
#endif
#endif
