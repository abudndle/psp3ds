#include <3ds.h>
#include <citro2d.h>
#include <stdint.h>
#include "psp_scene.h"
#include "psp_scene_batch.h"
#include "psp_ge_subset.h"
#include "psp_ge_triangles.h"
#include "psp_runtime_tuning.h"
#include "psp_video_layout.h"
/* Citro2D submits bounded rectangle primitives to the PICA200 GPU on
 * both original and New 3DS using an original synthetic scene. */
static PspScene scene;
static PspSceneBatch batch;
static PspGeSubsetResult ge_result;
static PspVideoLayout scene_layout;
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
 /* The same backend-neutral viewport mapping also supports native PSP
  * 480x272 frames on top or bottom displays when a GE renderer exists. */
 if (psp_video_layout_init(PSP_TARGET_OLD3DS,
         PSP_VIDEO_DISPLAY_3DS_BOTTOM,256,192,&scene_layout)!=0) return -1;
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
 /* Scale PSP-style batches through tested backend-neutral geometry,
  * rather than retaining a hard-coded OpenGL-style scale factor. */
 C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
 C2D_TargetClear(target,C2D_Color32(0,0,0,255));
 C2D_SceneBegin(target);
 int ok=1;
 for(unsigned i=0;i<ge_result.batch.count;i++){
  const PspSceneRect *r=&ge_result.batch.rects[i];
  const PspVideoRect src={r->x,r->y,r->w,r->h};
  PspVideoRect dst;
  if (psp_video_rect_project(&scene_layout,&src,&dst)!=0 ||
      !C2D_DrawRectSolid((float)dst.x,(float)dst.y,0.0f,
                        (float)dst.w,(float)dst.h,color555(r->bgr555))){
   ok=0;break;
  }
 }
 C3D_FrameEnd(0);
 return ok?0:-1;
}
/* Through-mode GE triangle diagnostic. Model-aware polygon count exercises
 * PICA200's native path, not a PPSSPP game or desktop GPU API. */
static void write_vertex16(uint8_t *p,uint16_t c,uint16_t x,uint16_t y,uint16_t z){
 const uint16_t v[4]={c,x,y,z};
 for(unsigned k=0;k<4;k++){
  p[k*2]=(uint8_t)v[k];
  p[k*2+1]=(uint8_t)(v[k]>>8);
 }
}
int psp_3ds_triangle_draw(uint32_t frame,PspTarget target_model,
                           uint16_t requested_triangle_limit){
 if(!initialized||!target)return -1;
 PspRuntimeTuning tuning;
 if(psp_runtime_tuning_choose(target_model,&tuning)!=0 ||
    (target_model!=PSP_TARGET_OLD3DS && target_model!=PSP_TARGET_NEW3DS))
  return -1;
 if(requested_triangle_limit==0u)return -1;
 const unsigned count=requested_triangle_limit<tuning.triangle_demo_budget?
                 requested_triangle_limit:tuning.triangle_demo_budget;
 if(!count || count>PSP_GE_TRIANGLE_MAX_TRIANGLES)return -1;
 static uint8_t guest_vertices[PSP_GE_TRIANGLE_MAX_TRIANGLES*3u*8u];
 const uint32_t words[6]={
  0x10080000u,0x01000000u,0x12800114u,
  0x04030000u | (3u*count),0x0f000000u,0x0c000000u
 };
 for(unsigned i=0;i<count;i++){
  const uint16_t x=(uint16_t)(8u+(i%6u)*78u);
  const uint16_t y=(uint16_t)(8u+(i/6u)*57u+((frame/16u)%8u));
  uint8_t *v=&guest_vertices[i*24u];
  write_vertex16(v+0u,0x801fu,x,y,1);
  write_vertex16(v+8u,0x83e0u,(uint16_t)(x+52u),(uint16_t)(y+4u),2);
  write_vertex16(v+16u,0xfc00u,(uint16_t)(x+26u),(uint16_t)(y+43u),3);
 }
 PspGeTriangleResult decoded;
 PspVideoLayout layout;
 if(psp_ge_triangles_decode(words,6,guest_vertices,count*24u,
                            0x08000000u,&decoded)!=PSP_GE_TRIANGLE_OK ||
    decoded.count!=count ||
    psp_video_layout_init(target_model,PSP_VIDEO_DISPLAY_3DS_BOTTOM,
                          PSP_GE_TRIANGLE_WIDTH,PSP_GE_TRIANGLE_HEIGHT,
                          &layout)!=0)return -1;
 C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
 C2D_TargetClear(target,C2D_Color32(0,0,0,255));
 C2D_SceneBegin(target);
 int ok=1;
 for(unsigned i=0;i<decoded.count;i++){
  const PspGeTriangle *t=&decoded.triangles[i];
  float x[3],y[3];
  for(unsigned v=0;v<3;v++){
   x[v]=(float)layout.viewport_x+
        (float)t->v[v].x*layout.viewport_width/layout.source_width;
   y[v]=(float)layout.viewport_y+
        (float)t->v[v].y*layout.viewport_height/layout.source_height;
  }
  if(!C2D_DrawTriangle(x[0],y[0],color555(t->v[0].color555),
                       x[1],y[1],color555(t->v[1].color555),
                       x[2],y[2],color555(t->v[2].color555),0.0f)){
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
