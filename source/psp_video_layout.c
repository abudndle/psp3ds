#include "psp_video_layout.h"
#include <string.h>

int psp_video_layout_init(PspTarget target, PspVideoDisplay display,
                          uint16_t sw, uint16_t sh, PspVideoLayout *out) {
 if (out) memset(out, 0, sizeof(*out));
 if (!out || !sw || !sh || sw > PSP_VIDEO_PSP_WIDTH ||
     sh > PSP_VIDEO_PSP_HEIGHT) return -1;
 uint16_t width=0, height=0;
 PspVideoBackend backend;
 switch (target) {
  case PSP_TARGET_DS:
  case PSP_TARGET_DS_SLOT2:
  case PSP_TARGET_DSI:
   if (display != PSP_VIDEO_DISPLAY_DS_MAIN) return -1;
   width=256; height=192; backend=PSP_VIDEO_BACKEND_DS_TILES;
   break;
  case PSP_TARGET_OLD3DS:
  case PSP_TARGET_NEW3DS:
   if (display == PSP_VIDEO_DISPLAY_3DS_TOP) {
    width=400; height=240;
   } else if (display == PSP_VIDEO_DISPLAY_3DS_BOTTOM) {
    width=320; height=240;
   } else return -1;
   backend=PSP_VIDEO_BACKEND_PICA200;
   break;
  default: return -1;
 }
 uint32_t vw=width, vh=height;
 if ((uint32_t)width * sh <= (uint32_t)height * sw) {
  vh=(uint32_t)width*sh/sw;
  if (!vh) vh=1;
 } else {
  vw=(uint32_t)height*sw/sh;
  if (!vw) vw=1;
 }
 out->target=target;
 out->display=display;
 out->backend=backend;
 out->source_width=sw;
 out->source_height=sh;
 out->screen_width=width;
 out->screen_height=height;
 out->viewport_width=(uint16_t)vw;
 out->viewport_height=(uint16_t)vh;
 out->viewport_x=(uint16_t)((width-vw)/2u);
 out->viewport_y=(uint16_t)((height-vh)/2u);
 return 0;
}

static uint32_t project_edge(uint32_t value,uint32_t dest,uint32_t source) {
 return (value*dest + source/2u)/source;
}
int psp_video_rect_project(const PspVideoLayout *layout,
                           const PspVideoRect *source, PspVideoRect *out) {
 if (out) memset(out, 0, sizeof(*out));
 if (!layout || !source || !out || !layout->source_width ||
     !layout->source_height || !layout->viewport_width ||
     !layout->viewport_height || !source->w || !source->h ||
     source->x >= layout->source_width ||
     source->y >= layout->source_height ||
     (uint32_t)source->x+source->w > layout->source_width ||
     (uint32_t)source->y+source->h > layout->source_height ||
     (uint32_t)layout->viewport_x+layout->viewport_width > layout->screen_width ||
     (uint32_t)layout->viewport_y+layout->viewport_height > layout->screen_height)
  return -1;
 uint32_t x0=project_edge(source->x,layout->viewport_width,layout->source_width);
 uint32_t x1=project_edge((uint32_t)source->x+source->w,layout->viewport_width,layout->source_width);
 uint32_t y0=project_edge(source->y,layout->viewport_height,layout->source_height);
 uint32_t y1=project_edge((uint32_t)source->y+source->h,layout->viewport_height,layout->source_height);
 /* A subpixel source primitive must not vanish entirely on a small LCD. */
 if (x0==x1) { if (x1<layout->viewport_width) ++x1; else --x0; }
 if (y0==y1) { if (y1<layout->viewport_height) ++y1; else --y0; }
 out->x=(uint16_t)(layout->viewport_x+x0);
 out->y=(uint16_t)(layout->viewport_y+y0);
 out->w=(uint16_t)(x1-x0);
 out->h=(uint16_t)(y1-y0);
 return 0;
}
