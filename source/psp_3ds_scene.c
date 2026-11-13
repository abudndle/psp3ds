#include <3ds.h>
#include <citro2d.h>
#include <stdint.h>
#include "psp_scene.h"
#include "psp_scene_batch.h"
#include "psp_ge_subset.h"
/* Citro2D submits bounded rectangle primitives to the PICA200 GPU on
 * both original and New 3DS using an original synthetic scene. */
static PspScene scene;
static PspSceneBatch batch;
static PspGeSubsetResult ge_result;
static uint8_t vertices[PSP_SCENE_MAX_RECTS * 16u];
static uint32_t ge_words[PSP_GE_SUBSET_MAX_COMMANDS];
static C3D_RenderTarget *target;
static int initialized;
static uint32_t color555(uint16_t v){
 const unsigned r=(v&31u)*255u/31u;
 const unsigned g=((v>>5)&31u)*255u/31u;
 const unsigned b=((v>>10)&31u)*255u/31u;
 return C2D_Color32(r,g,b,255u);
}
int psp_3ds_scene_init(void){
 if(initialized)return 0;
 if(!C3D_Init(C3D_DEFAULT_CMDBUF_SIZE))return -1;
 if(!C2D_Init(C2D_DEFAULT_MAX_OBJECTS)){
  C3D_Fini();return -1;
 }
 C2D_Prepare();
 target=C2D_CreateScreenTarget(GFX_BOTTOM,GFX_LEFT);
 if(!target){C2D_Fini();C3D_Fini();return -1;}
 initialized=1;
 return 0;
}
int psp_3ds_scene_draw(uint32_t frame){
 if(!initialized || !target || psp_scene_build(&scene,frame)!=0 ||
    psp_scene_batch_build(&scene,&batch)!=0)return -1;
 /* Encode the synthetic scene using the supported PSP vertex layout. */
 if (batch.count > PSP_GE_SUBSET_MAX_SPRITES ||
     batch.count + 5u > PSP_GE_SUBSET_MAX_COMMANDS) return -1;
 ge_words[0] = 0x10080000u; /* BASE for PSP RAM at 0x08000000. */
 ge_words[1] = 0x01000000u;
 ge_words[2] = 0x12800114u;
 for (unsigned i = 0; i < batch.count; i++) {
  const PspSceneRect *r = &batch.rects[i];
  const uint16_t color = r->bgr555 | 0x8000u;
  const uint16_t pair[] = {color, r->x, r->y, 0,
                           color, r->x + r->w, r->y + r->h, 0};
  for (unsigned field = 0; field < 8; field++) {
   vertices[i * 16u + field * 2u] = (uint8_t)pair[field];
   vertices[i * 16u + field * 2u + 1u] = (uint8_t)(pair[field] >> 8);
  }
  ge_words[3u + i] = 0x04060002u;
 }
 ge_words[batch.count + 3u] = 0x0f000000u;
 ge_words[batch.count + 4u] = 0x0c000000u;
 if (psp_ge_subset_translate_memory(ge_words, batch.count + 5u, vertices,
                                    batch.count * 16u, 0x08000000u,
                                    &ge_result) != PSP_GE_SUBSET_OK) return -1;
 /* DS scene logical coordinates are 256x192; bottom 3DS LCD is 320x240.
  * Integer ratio 5/4 keeps the whole scene visible without cropping. */
 C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
 C2D_TargetClear(target,C2D_Color32(0,0,0,255));
 C2D_SceneBegin(target);
 int ok=1;
 for(unsigned i=0;i<ge_result.batch.count;i++){
  const PspSceneRect *r=&ge_result.batch.rects[i];
  const float scale=1.25f;
  if(!C2D_DrawRectSolid(r->x*scale,r->y*scale,0.0f,
                        r->w*scale,r->h*scale,color555(r->bgr555))){
   ok=0;break;
  }
 }
 C3D_FrameEnd(0);
 return ok?0:-1;
}
void psp_3ds_scene_fini(void){
 if(!initialized)return;
 C2D_Fini();
 C3D_Fini();
 target=0;
 initialized=0;
}
