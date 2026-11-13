#include <3ds.h>
#include <citro2d.h>
#include <stdint.h>
#include "psp_scene.h"
#include "psp_scene_batch.h"
/* Citro2D submits bounded rectangle primitives to the PICA200 GPU on
 * both original and New 3DS. This is an ORIGINAL synthetic scene and
 * is neither PSP GE translation nor commercial game rendering. */
static PspScene scene;
static PspSceneBatch batch;
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
 /* DS scene logical coordinates are 256x192; bottom 3DS LCD is 320x240.
  * Integer ratio 5/4 keeps the whole scene visible without cropping. */
 C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
 C2D_TargetClear(target,C2D_Color32(0,0,0,255));
 C2D_SceneBegin(target);
 int ok=1;
 for(unsigned i=0;i<batch.count;i++){
  const PspSceneRect *r=&batch.rects[i];
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
