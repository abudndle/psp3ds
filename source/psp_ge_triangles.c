#include "psp_ge_triangles.h"
#include <string.h>

static uint16_t get16(const uint8_t *p){
 return (uint16_t)((uint16_t)p[0] | ((uint16_t)p[1]<<8));
}
int psp_ge_triangles_decode(const uint32_t *words,size_t count,
                            const uint8_t *guest,size_t size,
                            uint32_t guest_base,PspGeTriangleResult *out){
 if(out)memset(out,0,sizeof(*out));
 if(!out||!words||!guest||!count||!size||
    count>PSP_GE_TRIANGLE_MAX_COMMANDS)return PSP_GE_TRIANGLE_BAD_ARGUMENT;
 uint32_t base=0,address=0;
 int have_addr=0,have_type=0,finished=0,ended=0;
 PspGeTriangleResult staged={0};
 for(size_t i=0;i<count;i++){
  const uint32_t word=words[i],arg=word&0x00ffffffu;
  const unsigned op=word>>24;
  if(ended)return PSP_GE_TRIANGLE_MALFORMED;
  switch(op){
   case 0x00: /* exact NOP only before FINISH */
    if(arg||finished)return PSP_GE_TRIANGLE_UNSUPPORTED;
    break;
   case 0x10: /* BASE high address bits */
    if(finished||arg & ~0x000f0000u)return PSP_GE_TRIANGLE_UNSUPPORTED;
    base=arg<<8;
    break;
   case 0x01: /* guest VADDR */
    if(finished)return PSP_GE_TRIANGLE_MALFORMED;
    address=base|arg;
    have_addr=1;
    break;
   case 0x12: /* color 5551, signed X/Y/Z 16, through-mode */
    if(finished||arg!=0x00800114u)return PSP_GE_TRIANGLE_UNSUPPORTED;
    have_type=1;
    break;
   case 0x04: {
    if(finished||!have_addr||!have_type)return PSP_GE_TRIANGLE_MALFORMED;
    const unsigned prim=(arg>>16)&7u,verts=arg&0xffffu;
    if(prim!=3u||!verts||verts%3u!=0u)return PSP_GE_TRIANGLE_UNSUPPORTED;
    const unsigned tris=verts/3u;
    if(tris>PSP_GE_TRIANGLE_MAX_TRIANGLES-staged.count)
     return PSP_GE_TRIANGLE_OVERFLOW;
    const uint64_t bytes=(uint64_t)verts*8u;
    if((address&1u)||address<guest_base)return PSP_GE_TRIANGLE_MALFORMED;
    const uint64_t off=(uint64_t)address-guest_base;
    if(off>size||bytes>(uint64_t)size-off||
       (uint64_t)address+bytes>UINT32_MAX)
     return PSP_GE_TRIANGLE_MALFORMED;
    for(unsigned t=0;t<tris;t++){
     PspGeTriangle *dst=&staged.triangles[staged.count+t];
     for(unsigned v=0;v<3;v++){
      const uint8_t *p=guest+(size_t)off+(size_t)(t*3u+v)*8u;
      const uint16_t c=get16(p);
      const uint16_t x=get16(p+2u),y=get16(p+4u),z=get16(p+6u);
      if(!(c&0x8000u))return PSP_GE_TRIANGLE_UNSUPPORTED;
      if(x>=PSP_GE_TRIANGLE_WIDTH||y>=PSP_GE_TRIANGLE_HEIGHT)
       return PSP_GE_TRIANGLE_MALFORMED;
      dst->v[v]=(PspGeTriangleVertex){x,y,z,(uint16_t)(c&0x7fffu)};
     }
    }
    staged.count+=tris;
    address+=(uint32_t)bytes;
    break;
   }
   case 0x0f: /* FINISH */
    if(arg||finished)return PSP_GE_TRIANGLE_MALFORMED;
    finished=1;
    break;
   case 0x0c: /* END */
    if(arg||!finished)return PSP_GE_TRIANGLE_MALFORMED;
    ended=1;
    break;
   default: return PSP_GE_TRIANGLE_UNSUPPORTED;
  }
 }
 if(!ended||!staged.count)return PSP_GE_TRIANGLE_MALFORMED;
 staged.consumed_commands=(uint32_t)count;
 *out=staged;
 return PSP_GE_TRIANGLE_OK;
}
