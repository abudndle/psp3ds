#include "psp_scene.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
int main(void){
 PspScene s,t;uint16_t c;
 assert(psp_scene_build(&s,0)==0);
 assert(s.scroll_x==0&&s.actor_x==32&&s.actor_y==70);
 assert(s.map[0]==0 && s.map[20*32]==1);
 assert(psp_scene_color(&s,0,0,&c)==0&&c==s.palette[1]);
 assert(psp_scene_color(&s,40,70,&c)==0&&c==s.palette[1]);
 assert(psp_scene_color(&s,32,70,&c)==0&&c==s.palette[3]);
 assert(psp_scene_color(&s,0,191,&c)==0&&c==s.palette[2]);
 assert(psp_scene_color(&s,256,0,&c)==-1);
 assert(psp_scene_color(&s,0,192,&c)==-1);
 assert(psp_scene_color(&s,0,0,NULL)==-1);
 assert(psp_scene_build(&t,0)==0&&memcmp(&s,&t,sizeof s)==0);
 assert(psp_scene_build(&t,3)==0&&t.scroll_x==6&&t.actor_x==35);
 assert(psp_scene_build(NULL,0)==-1);
 uint32_t count[4]={0};
 for(unsigned y=0;y<192;y++)for(unsigned x=0;x<256;x++){
  assert(psp_scene_color(&s,x,y,&c)==0);
  int matched=0;
  for(unsigned i=1;i<=3;i++)if(c==s.palette[i]){count[i]++;matched=1;break;}
  assert(matched);
 }
 assert(count[3]>=64u);
 assert(count[1]+count[2]+count[3]==256u*192u);
 puts("PASS: common deterministic tile scene, scrolling, actor, complete raster and bounds");
 return 0;
}
