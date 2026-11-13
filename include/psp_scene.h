#ifndef PSP_SCENE_H
#define PSP_SCENE_H
#include <stddef.h>
#include <stdint.h>
/* Original synthetic scrolling background + moving 8x8 player marker.
 * Single semantic scene used on DS/DSi and original/New 3DS.
 * All outputs are deterministic and game-asset-free. */
#define PSP_SCENE_COLS 32u
#define PSP_SCENE_ROWS 32u
#define PSP_SCENE_TILES 1024u
typedef struct {
 uint16_t map[PSP_SCENE_TILES];
 uint16_t palette[4]; /* DS BGR555, index 0 transparent/black. */
 uint8_t tile4[3][32]; /* 8x8, 4bpp tile source. */
 uint16_t scroll_x;
 uint16_t scroll_y;
 uint16_t actor_x;
 uint16_t actor_y;
} PspScene;
int psp_scene_build(PspScene *out,uint32_t frame);
int psp_scene_color(const PspScene *scene,unsigned x,unsigned y,uint16_t *rgb555);
#endif
