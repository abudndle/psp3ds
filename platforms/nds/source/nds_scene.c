#include <nds.h>
#include <string.h>
#include "psp_scene.h"
/* DS main-screen hardware 2D backend.
 * Three 4bpp tiles and a 32x32 map use 2,144 bytes of BG VRAM.
 * The actor is a genuine hardware OAM 8x8 sprite in VRAM B.
 * The main 2D engine is independent of the sub-screen diagnostic console. */
static PspScene scene;
static int background=-1;
static u16 *actor_gfx;
int psp_nds_scene_init(void){
 if(psp_scene_build(&scene,0)!=0)return -1;
 videoSetMode(MODE_0_2D);
 vramSetBankA(VRAM_A_MAIN_BG);
 vramSetBankB(VRAM_B_MAIN_SPRITE);
 background=bgInitHidden(2,BgType_Text4bpp,BgSize_T_256x256,0,1);
 if(background<0)return -1;
 memcpy(bgGetGfxPtr(background),scene.tile4,sizeof(scene.tile4));
 memcpy(bgGetMapPtr(background),scene.map,sizeof(scene.map));
 memcpy(BG_PALETTE,scene.palette,sizeof(scene.palette));
 oamInit(&oamMain,SpriteMapping_1D_128,false);
 actor_gfx=oamAllocateGfx(&oamMain,SpriteSize_8x8,SpriteColorFormat_16Color);
 if(!actor_gfx)return -1;
 for(unsigned i=0;i<16;i++)actor_gfx[i]=0x3333u;
 memcpy(SPRITE_PALETTE,scene.palette,sizeof(scene.palette));
 oamSet(&oamMain,0,scene.actor_x,scene.actor_y,0,0,
        SpriteSize_8x8,SpriteColorFormat_16Color,actor_gfx,
        -1,false,false,false,false,false);
 bgSetPriority(background,2);
 bgShow(background);
 return 0;
}
void psp_nds_scene_update(uint32_t frame){
 if(background<0||!actor_gfx)return;
 if(psp_scene_build(&scene,frame)!=0)return;
 bgSetScroll(background,scene.scroll_x,scene.scroll_y);
 bgUpdate();
 oamSetXY(&oamMain,0,scene.actor_x,scene.actor_y);
 oamUpdate(&oamMain);
}
