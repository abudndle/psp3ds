#include <3ds.h>
#include <stdint.h>
#include "psp_scene.h"
/* Explicit software FRAMEBUFFER fallback for a synthetic scene.
 * NOT a PICA200 GPU sprite backend or PSP GE renderer. This remains
 * behind A-button opt-in so the old deterministic boot marker persists. */
static PspScene scene;
int psp_3ds_scene_draw(uint32_t frame) {
 if(psp_scene_build(&scene,frame))return -1;
 u16 w=0,h=0;
 u8 *fb=gfxGetFramebuffer(GFX_BOTTOM,GFX_LEFT,&w,&h);
 if(!fb||!w||!h)return -1;
 /* libctru BGR8 framebuffer is rotated. Scene logical coordinates are
  * mapped from the native rotated display, without unbounded allocations. */
 for(unsigned py=0;py<h;py++){
  unsigned sx=(py*256u)/h;
  for(unsigned px=0;px<w;px++){
   unsigned sy=(px*192u)/w;
   uint16_t bgr=0;
   if(psp_scene_color(&scene,sx,sy,&bgr))return -1;
   size_t at=((size_t)py*w+px)*3u;
   fb[at]=(u8)(((bgr>>10)&31u)*255u/31u);
   fb[at+1]=(u8)(((bgr>>5)&31u)*255u/31u);
   fb[at+2]=(u8)((bgr&31u)*255u/31u);
  }
 }
 return 0;
}
