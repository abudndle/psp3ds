#include "psp_scene_batch.h"
#include <string.h>
static int add(PspSceneBatch *out,int x,int y,int w,int h,uint16_t color){
 if(w<=0||h<=0)return 0;
 if(x<0){w+=x;x=0;}if(y<0){h+=y;y=0;}
 if(x>=256||y>=192||w<=0||h<=0)return 0;
 if(w>256-x)w=256-x;
 if(h>192-y)h=192-y;
 if(out->count>=PSP_SCENE_MAX_RECTS)return -1;
 PspSceneRect *r=&out->rects[out->count++];
 r->x=(uint16_t)x;r->y=(uint16_t)y;
 r->w=(uint16_t)w;r->h=(uint16_t)h;r->bgr555=color;
 return 0;
}
int psp_scene_batch_build(const PspScene *s,PspSceneBatch *out){
 if(!s||!out)return -1;
 memset(out,0,sizeof(*out));
 if(s->scroll_y || s->scroll_x>=256u ||
    s->actor_x>255u || s->actor_y>191u)return -1;
 /* The synthetic map is specified as sky through tile row 17, ground
  * thereafter. Check every tile before using a two-layer batching plan. */
 for(unsigned y=0;y<32u;y++)for(unsigned x=0;x<32u;x++){
  unsigned want=y<18u?0u:(((x+y)%8u)==0u?2u:1u);
  if(s->map[y*32u+x]!=want)return -1;
 }
 if(add(out,0,0,256,192,s->palette[1]) ||
    add(out,0,144,256,48,s->palette[2]))return -1;
 /* Draw only sparse ground stripes instead of painting 49,152 pixels.
  * Clip partially visible tiles at the right and left edges. */
 unsigned phase=s->scroll_x&7u;
 for(unsigned ty=18u;ty<24u;ty++) {
  int y=(int)(ty*8u);
  for(int x=-(int)phase;x<256;x+=8){
   unsigned tx=(unsigned)(((x+(int)s->scroll_x)/8)&31);
   if((tx+ty)%8u==0u &&
      add(out,x,y,8,8,s->palette[3]))return -1;
  }
 }
 return add(out,s->actor_x,s->actor_y,8,8,s->palette[3]);
}
int psp_scene_batch_color(const PspSceneBatch *batch,
                          unsigned x,unsigned y,uint16_t *color){
 if(!batch||!color||x>=256u||y>=192u||
    !batch->count||batch->count>PSP_SCENE_MAX_RECTS)return -1;
 int found=0;
 for(unsigned i=0;i<batch->count;i++){
  const PspSceneRect *r=&batch->rects[i];
  if(!r->w||!r->h||r->x>256u||r->y>192u||
     (unsigned)r->x+r->w>256u||(unsigned)r->y+r->h>192u)return -1;
  if(x>=r->x&&x<(unsigned)r->x+r->w&&
     y>=r->y&&y<(unsigned)r->y+r->h){*color=r->bgr555;found=1;}
 }
 return found?0:-1;
}
