#ifndef PSP_SCENE_BATCH_H
#define PSP_SCENE_BATCH_H
#include "psp_scene.h"
#include <stdint.h>
#define PSP_SCENE_MAX_RECTS 64u
typedef struct {
 uint16_t x,y,w,h;
 uint16_t bgr555;
} PspSceneRect;
typedef struct {
 PspSceneRect rects[PSP_SCENE_MAX_RECTS];
 unsigned count;
} PspSceneBatch;
/* Ordered opaque rectangles: sky, ground, stripe tiles, actor.
 * Original 256x192 scene, independent of GPU APIs. */
int psp_scene_batch_build(const PspScene *scene,PspSceneBatch *out);
int psp_scene_batch_color(const PspSceneBatch *batch,
                          unsigned x,unsigned y,uint16_t *color);
#endif
