#include <nds.h>
#include <nds/arm9/videoGL.h>
#include <stdint.h>
#include "psp_ge_triangles.h"

/* Native Nintendo DS geometry engine. The libnds gl* names program actual
 * DS hardware registers; they are not a desktop OpenGL dependency.
 * Works identically on DS, verified DS+Slot-2 and DSi/DSi XL.
 * This demo draws only untextured through-mode triangle fixtures. */
static const uint32_t commands[]={
 0x10080000u,0x01000000u,0x12800114u,
 0x04030003u,0x0f000000u,0x0c000000u
};
static const uint8_t vertices[]={
 0x1f,0x80,20,0,20,0,1,0,
 0xe0,0x83,180,0,30,0,2,0,
 0x00,0xfc,100,0,150,0,3,0
};
static PspGeTriangleResult triangles;
static int initialized;

int psp_nds_triangles_init(void){
 if(initialized)return 0;
 if(psp_ge_triangles_decode(commands,6,vertices,sizeof vertices,
                            0x08000000u,&triangles)!=PSP_GE_TRIANGLE_OK)
  return -1;
 videoSetMode(MODE_0_3D);
 if(!glInit()){
  videoSetMode(MODE_0_2D);
  return -1;
 }
 glViewport(0,0,255,191);
 glClearColor(0,0,0,31);
 glClearDepth(GL_MAX_DEPTH);
 glMatrixMode(GL_PROJECTION);
 glLoadIdentity();
 glMatrixMode(GL_MODELVIEW);
 glLoadIdentity();
 initialized=1;
 return 0;
}
void psp_nds_triangles_update(void){
 if(!initialized)return;
 glPolyFmt(POLY_ALPHA(31)|POLY_CULL_NONE);
 glBegin(GL_TRIANGLES);
 for(unsigned t=0;t<triangles.count;t++){
  for(unsigned v=0;v<3;v++){
   const PspGeTriangleVertex *p=&triangles.triangles[t].v[v];
   /* Vertex16 takes signed 4.12 normalized clip coordinates, not pixels.
    * No floating point and no GL/Vulkan/D3D runtime needed on ARM9.
    * Geometry is intentionally flat: PSP depth/VFPU still unimplemented. */
   const int x=(int)((uint32_t)p->x*8192u/PSP_GE_TRIANGLE_WIDTH)-4096;
   const int y=4096-(int)((uint32_t)p->y*8192u/PSP_GE_TRIANGLE_HEIGHT);
   glColor((rgb)p->color555);
   glVertex3v16((v16)x,(v16)y,(v16)0);
  }
 }
 glEnd();
 glFlush(0);
}
void psp_nds_triangles_shutdown(void){
 if(!initialized)return;
 glDeinit();
 videoSetMode(MODE_0_2D);
 initialized=0;
}
