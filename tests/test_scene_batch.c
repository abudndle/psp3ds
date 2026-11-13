#include "psp_scene.h"
#include "psp_scene_batch.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
int main(void){
 const uint32_t frames[]={0u,1u,3u,7u,15u,31u,63u,127u,128u,511u};
 for(unsigned k=0;k<sizeof(frames)/sizeof(frames[0]);k++){
  PspScene scene;PspSceneBatch batch;
  assert(psp_scene_build(&scene,frames[k])==0);
  assert(psp_scene_batch_build(&scene,&batch)==0);
  assert(batch.count>=3u&&batch.count<=PSP_SCENE_MAX_RECTS);
  assert(batch.rects[0].x==0u&&batch.rects[0].w==256u);
  assert(batch.rects[0].h==192u);
  for(unsigned y=0;y<192u;y++)for(unsigned x=0;x<256u;x++){
   uint16_t a=0,b=0;
   assert(psp_scene_color(&scene,x,y,&a)==0);
   assert(psp_scene_batch_color(&batch,x,y,&b)==0);
   assert(a==b);
  }
 }
 PspScene scene;PspSceneBatch batch;uint16_t color=0;
 assert(psp_scene_build(&scene,0)==0);
 assert(psp_scene_batch_build(NULL,&batch)==-1);
 assert(psp_scene_batch_build(&scene,NULL)==-1);
 assert(psp_scene_batch_color(NULL,0,0,&color)==-1);
 assert(psp_scene_batch_color(&batch,256,0,&color)==-1);
 scene.scroll_y=1;
 assert(psp_scene_batch_build(&scene,&batch)==-1);
 assert(psp_scene_build(&scene,0)==0);
 scene.map[0]=99;
 assert(psp_scene_batch_build(&scene,&batch)==-1);
 puts("PASS: GPU rectangle batch pixel parity for ten frames, bounds and fail-closed invalid scene");
 return 0;
}
