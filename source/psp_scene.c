#include "psp_scene.h"
#include <string.h>
/* Tile indices 0 (sky), 1 (ground), 2 (stripe); index 3 only when
 * drawing the 8x8 actor marker in the framebuffer raster adapter. */
int psp_scene_build(PspScene *out,uint32_t frame) {
 if(!out)return -1;
 memset(out,0,sizeof(*out));
 out->palette[0]=0u;
 out->palette[1]=0x7e00u; /* light sky; DS BGR555 */
 out->palette[2]=0x01afu; /* grass */
 out->palette[3]=0x7c1fu; /* magenta moving marker/stripe */
 for(unsigned t=0;t<3;t++)
  for(unsigned i=0;i<32;i++)out->tile4[t][i]=(uint8_t)((t+1u)|((t+1u)<<4));
 for(unsigned y=0;y<PSP_SCENE_ROWS;y++)
  for(unsigned x=0;x<PSP_SCENE_COLS;x++)
   out->map[y*PSP_SCENE_COLS+x]=(uint16_t)(y>=18u?((x+y)%8u==0u?2u:1u):0u);
 out->scroll_x=(uint16_t)((frame*2u)%256u);
 out->scroll_y=0u;
 out->actor_x=(uint16_t)(32u+(frame%160u));
 out->actor_y=(uint16_t)(70u+((frame/4u)%24u));
 return 0;
}
int psp_scene_color(const PspScene *scene,unsigned x,unsigned y,uint16_t *rgb555){
 if(!scene||!rgb555||x>=256u||y>=192u)return -1;
 if(x>=scene->actor_x && x<(unsigned)scene->actor_x+8u &&
    y>=scene->actor_y && y<(unsigned)scene->actor_y+8u){
  *rgb555=scene->palette[3];
  return 0;
 }
 unsigned sx=(x+scene->scroll_x)&255u;
 unsigned sy=(y+scene->scroll_y)&255u;
 unsigned tile=(unsigned)scene->map[(sy/8u)*32u+(sx/8u)];
 if(tile>=3u)return -1;
 *rgb555=scene->palette[tile+1u];
 return 0;
}
