#include <3ds.h>
#include <citro2d.h>
#include <stdint.h>
#include "psp_scene.h"
#include "psp_scene_batch.h"
#include "psp_ge_subset.h"
/* Citro2D submits bounded rectangle primitives to the PICA200 GPU on
 * both original and New 3DS. This is an ORIGINAL synthetic scene and
 * is neither PSP GE translation nor commercial game rendering. */
static PspScene scene;
static PspSceneBatch batch;
static PspGeSubsetResult ge_result;
static PspGeDecodedSprite decoded[PSP_SCENE_MAX_RECTS];
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
 /* Bridge a strictly synthetic predecoded-vertex GE sprite fixture into
  * the same bounded GPU rectangle list. PSP vertex formats, textures,
  * framebuffer state and genuine guest display lists are NOT supported. */
 if(batch.count>PSP_GE_SUBSET_MAX_SPRITES ||
    batch.count+2u>PSP_GE_SUBSET_MAX_COMMANDS)return -1;
 for(unsigned i=0;i<batch.count;i++){
  const PspSceneRect *r=&batch.rects[i];
  decoded[i]=(PspGeDecodedSprite){r->x,r->y,r->w,r->h,r->bgr555};
  ge_words[i]=UINT32_C(0x04060002);
 }
 ge_words[batch.count]=UINT32_C(0x0f000000);
 ge_words[batch.count+1u]=UINT32_C(0x0c000000);
 if(psp_ge_subset_translate(ge_words,batch.count+2u,decoded,
                            batch.count,&ge_result)!=PSP_GE_SUBSET_OK)return -1;
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
