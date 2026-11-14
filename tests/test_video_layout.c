#include "psp_video_layout.h"
#include <assert.h>
#include <stdio.h>

static void verify(PspTarget target, PspVideoDisplay display,
                   unsigned x,unsigned y,unsigned w,unsigned h,
                   PspVideoBackend backend) {
 PspVideoLayout l;
 assert(psp_video_layout_init(target,display,PSP_VIDEO_PSP_WIDTH,PSP_VIDEO_PSP_HEIGHT,&l)==0);
 assert(l.backend==backend);
 assert(l.viewport_x==x && l.viewport_y==y);
 assert(l.viewport_width==w && l.viewport_height==h);
 assert(l.viewport_x+l.viewport_width<=l.screen_width);
 assert(l.viewport_y+l.viewport_height<=l.screen_height);
 PspVideoRect whole={0,0,480,272}, projected;
 assert(psp_video_rect_project(&l,&whole,&projected)==0);
 assert(projected.x==x&&projected.y==y&&projected.w==w&&projected.h==h);
 PspVideoRect tiny={479,271,1,1};
 assert(psp_video_rect_project(&l,&tiny,&projected)==0);
 assert(projected.w>=1&&projected.h>=1);
 assert((unsigned)projected.x+projected.w<=l.screen_width);
 assert((unsigned)projected.y+projected.h<=l.screen_height);
 PspVideoRect bad={479,271,2,1};
 assert(psp_video_rect_project(&l,&bad,&projected)!=0);
 assert(!projected.w && !projected.h);
}
int main(void) {
 verify(PSP_TARGET_DS,PSP_VIDEO_DISPLAY_DS_MAIN,0,23,256,145,PSP_VIDEO_BACKEND_DS_TILES);
 verify(PSP_TARGET_DS_SLOT2,PSP_VIDEO_DISPLAY_DS_MAIN,0,23,256,145,PSP_VIDEO_BACKEND_DS_TILES);
 verify(PSP_TARGET_DSI,PSP_VIDEO_DISPLAY_DS_MAIN,0,23,256,145,PSP_VIDEO_BACKEND_DS_TILES);
 verify(PSP_TARGET_OLD3DS,PSP_VIDEO_DISPLAY_3DS_TOP,0,7,400,226,PSP_VIDEO_BACKEND_PICA200);
 verify(PSP_TARGET_NEW3DS,PSP_VIDEO_DISPLAY_3DS_TOP,0,7,400,226,PSP_VIDEO_BACKEND_PICA200);
 verify(PSP_TARGET_OLD3DS,PSP_VIDEO_DISPLAY_3DS_BOTTOM,0,29,320,181,PSP_VIDEO_BACKEND_PICA200);
 verify(PSP_TARGET_NEW3DS,PSP_VIDEO_DISPLAY_3DS_BOTTOM,0,29,320,181,PSP_VIDEO_BACKEND_PICA200);
 PspVideoLayout l;
 assert(psp_video_layout_init(PSP_TARGET_DS,PSP_VIDEO_DISPLAY_3DS_TOP,480,272,&l)==-1);
 assert(psp_video_layout_init(PSP_TARGET_NEW3DS,PSP_VIDEO_DISPLAY_DS_MAIN,480,272,&l)==-1);
 assert(psp_video_layout_init(PSP_TARGET_COUNT,PSP_VIDEO_DISPLAY_DS_MAIN,480,272,&l)==-1);
 assert(psp_video_layout_init(PSP_TARGET_DSI,PSP_VIDEO_DISPLAY_DS_MAIN,0,272,&l)==-1);
 assert(psp_video_layout_init(PSP_TARGET_DSI,PSP_VIDEO_DISPLAY_DS_MAIN,481,272,&l)==-1);
 assert(psp_video_layout_init(PSP_TARGET_DSI,PSP_VIDEO_DISPLAY_DS_MAIN,480,273,&l)==-1);
 assert(psp_video_layout_init(PSP_TARGET_DSI,PSP_VIDEO_DISPLAY_DS_MAIN,480,272,NULL)==-1);
 assert(psp_video_layout_init(PSP_TARGET_OLD3DS,PSP_VIDEO_DISPLAY_3DS_BOTTOM,256,192,&l)==0);
 assert(l.viewport_width==320&&l.viewport_height==240&&l.viewport_x==0&&l.viewport_y==0);
 PspVideoRect source={3,7,8,8}, actual;
 assert(psp_video_rect_project(&l,&source,&actual)==0);
 assert(actual.x==4&&actual.y==9&&actual.w==10&&actual.h==10);
 source=(PspVideoRect){0,0,0,5};
 assert(psp_video_rect_project(&l,&source,&actual)==-1);
 assert(!actual.w && !actual.h);
 assert(psp_video_rect_project(NULL,&source,&actual)==-1);
 assert(psp_video_rect_project(&l,&source,NULL)==-1);
 puts("PASS: PSP-native viewport and OpenGL-free rectangle projection on all DS/3DS family policies");
 return 0;
}
