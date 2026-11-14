#ifndef PSP_VIDEO_LAYOUT_H
#define PSP_VIDEO_LAYOUT_H

#include <stdint.h>
#include "psp_platform_budget.h"

#define PSP_VIDEO_PSP_WIDTH 480u
#define PSP_VIDEO_PSP_HEIGHT 272u

typedef enum {
 PSP_VIDEO_DISPLAY_DS_MAIN=0,
 PSP_VIDEO_DISPLAY_3DS_TOP=1,
 PSP_VIDEO_DISPLAY_3DS_BOTTOM=2
} PspVideoDisplay;

typedef enum {
 PSP_VIDEO_BACKEND_DS_TILES=0,
 PSP_VIDEO_BACKEND_PICA200=1
} PspVideoBackend;

typedef struct {
 PspTarget target;
 PspVideoDisplay display;
 PspVideoBackend backend;
 uint16_t source_width, source_height;
 uint16_t screen_width, screen_height;
 uint16_t viewport_x, viewport_y;
 uint16_t viewport_width, viewport_height;
} PspVideoLayout;

typedef struct {
 uint16_t x, y, w, h;
} PspVideoRect;

/* Calculates a centered, aspect-preserving integer viewport. This is
 * platform-neutral geometry: no OpenGL, Vulkan, framebuffer or heap. */
int psp_video_layout_init(PspTarget target, PspVideoDisplay display,
                          uint16_t source_width, uint16_t source_height,
                          PspVideoLayout *out);

/* Maps an in-bounds source rectangle using rounded pixel boundaries.
 * Rejects invalid rectangles; zeroes the result on failure. */
int psp_video_rect_project(const PspVideoLayout *layout,
                           const PspVideoRect *source, PspVideoRect *out);

#endif
