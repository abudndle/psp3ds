#include "psp_nintendo_input.h"
#include <string.h>

static int clamp_circle(int32_t v) {
 if (v > 156) return 156;
 if (v < -156) return -156;
 return (int)v;
}
static uint8_t axis_to_psp(int32_t v) {
 const int n=clamp_circle(v);
 /* Separate positive/negative spans so both edges are reachable and
  * centered input stays exactly 128. Bounds avoid integer overflow. */
 return (uint8_t)(n < 0 ? 128 + n * 128 / 156 :
                            128 + n * 127 / 156);
}
int psp_nintendo_ctrl_sample(PspTarget target, uint32_t native_keys,
                             int32_t circle_dx, int32_t circle_dy,
                             PspNintendoCtrl *out) {
 if (out) memset(out,0,sizeof(*out));
 if (!out || (unsigned)target >= PSP_TARGET_COUNT) return -1;
 uint32_t buttons=0;
 const uint32_t k=native_keys & PSP_NINTENDO_KEY_MASK;
 if (k & PSP_NINTENDO_KEY_A) buttons |= PSP_CTRL_CIRCLE;
 if (k & PSP_NINTENDO_KEY_B) buttons |= PSP_CTRL_CROSS;
 if (k & PSP_NINTENDO_KEY_X) buttons |= PSP_CTRL_TRIANGLE;
 if (k & PSP_NINTENDO_KEY_Y) buttons |= PSP_CTRL_SQUARE;
 if (k & PSP_NINTENDO_KEY_SELECT) buttons |= PSP_CTRL_SELECT;
 if (k & PSP_NINTENDO_KEY_START) buttons |= PSP_CTRL_START;
 if (k & PSP_NINTENDO_KEY_UP) buttons |= PSP_CTRL_UP;
 if (k & PSP_NINTENDO_KEY_DOWN) buttons |= PSP_CTRL_DOWN;
 if (k & PSP_NINTENDO_KEY_LEFT) buttons |= PSP_CTRL_LEFT;
 if (k & PSP_NINTENDO_KEY_RIGHT) buttons |= PSP_CTRL_RIGHT;
 if (k & PSP_NINTENDO_KEY_L) buttons |= PSP_CTRL_LTRIGGER;
 if (k & PSP_NINTENDO_KEY_R) buttons |= PSP_CTRL_RTRIGGER;
 int x=0,y=0;
 if (target==PSP_TARGET_OLD3DS || target==PSP_TARGET_NEW3DS) {
  x=clamp_circle(circle_dx);
  y=clamp_circle(circle_dy);
 } else {
  /* A physical DS/DSi has no circle pad, so let D-pad provide a
   * deterministic optional-style analog fallback. Opposites cancel. */
  x=((k & PSP_NINTENDO_KEY_RIGHT)!=0) -
    ((k & PSP_NINTENDO_KEY_LEFT)!=0);
  y=((k & PSP_NINTENDO_KEY_UP)!=0) -
    ((k & PSP_NINTENDO_KEY_DOWN)!=0);
  x*=156; y*=156;
 }
 out->buttons=buttons;
 out->lx=axis_to_psp(x);
 out->ly=axis_to_psp(-y); /* libctru circlePosition dy is positive up. */
 return 0;
}
