#include "psp_native_texture_stream.h"
#include <stdint.h>
#include <string.h>

static uint8_t ex5(unsigned v){return (uint8_t)((v<<3u)|(v>>2u));}
static uint8_t ex6(unsigned v){return (uint8_t)((v<<2u)|(v>>4u));}
static uint8_t ex4(unsigned v){return (uint8_t)((v<<4u)|v);}
static void pixel(const uint8_t *src,unsigned format,uint8_t *out){
 if(format==PSP_TEXTURE_8888){
  memcpy(out,src,4);return;
 }
 const unsigned v=(unsigned)src[0]|((unsigned)src[1]<<8u);
 if(format==PSP_TEXTURE_5650){
  out[0]=ex5(v&31u);out[1]=ex6((v>>5u)&63u);
  out[2]=ex5((v>>11u)&31u);out[3]=255u;
 }else if(format==PSP_TEXTURE_5551){
  out[0]=ex5(v&31u);out[1]=ex5((v>>5u)&31u);
  out[2]=ex5((v>>10u)&31u);out[3]=(v&0x8000u)?255u:0u;
 }else{
  out[0]=ex4(v&15u);out[1]=ex4((v>>4u)&15u);
  out[2]=ex4((v>>8u)&15u);out[3]=ex4((v>>12u)&15u);
 }
}
typedef struct {
 uint8_t bytes[PSP_TEXTURE_STREAM_SWIZZLE_BYTES];
 uint64_t block;
 uint32_t age;
 uint8_t valid;
} SwizzleSlot;

int psp_native_texture_decode_stream(
 const PspNativeTexture *t,uint64_t base,uint64_t available,
 const PspNativeTextureRegion *region,PspTextureStreamRead read,
 void *opaque,uint8_t *out,size_t stride,size_t capacity,
 PspTextureStreamStats *stats){
 if(stats)memset(stats,0,sizeof(*stats));
 if(!t||!region||!read||!out)return PSP_TEXTURE_BAD_ARGUMENT;
 if(t->format>PSP_TEXTURE_8888)return PSP_TEXTURE_UNSUPPORTED;
 if(t->swizzled>1u||!t->width||!t->height||
    t->width>512u||t->height>512u||
    t->buffer_width<t->width||t->buffer_width>2048u||
    !region->width||!region->height||region->x>=t->width||
    region->y>=t->height||
    (uint32_t)region->x+region->width>t->width||
    (uint32_t)region->y+region->height>t->height)return PSP_TEXTURE_BAD_ARGUMENT;
 const size_t bpp=t->format==PSP_TEXTURE_8888?4u:2u;
 const size_t pitch=(size_t)t->buffer_width*bpp;
 const size_t required_rows=t->swizzled?
    (((size_t)t->height+7u)/8u)*8u:t->height;
 const uint64_t required=(uint64_t)pitch*required_rows;
 if(t->swizzled&&(pitch%16u)!=0u)return PSP_TEXTURE_BAD_ARGUMENT;
 if(available<required||base>UINT64_MAX-required)return PSP_TEXTURE_TRUNCATED;
 const size_t line=(size_t)region->width*4u;
 if(stride<line||(region->height>1u&&
    stride>(SIZE_MAX-line)/(region->height-1u))||
    capacity<(size_t)(region->height-1u)*stride+line)
  return PSP_TEXTURE_TRUNCATED;
 /* The stack scratch is fixed even if caller holds a 512-MiB ROM file.
  * Never read an entire PSP texture just to emit a small DS tile. */
 uint8_t linear[PSP_TEXTURE_STREAM_LINEAR_BYTES];
 SwizzleSlot tiles[PSP_TEXTURE_STREAM_SWIZZLE_SLOTS];
 memset(tiles,0,sizeof tiles);
 uint32_t tick=0u;
 PspTextureStreamStats result={0};
 for(size_t row=0;row<region->height;row++){
  const size_t y=(size_t)region->y+row;
  uint8_t *destination=out+row*stride;
  if(!t->swizzled){
   const size_t n=(size_t)region->width*bpp;
   const uint64_t pos=(uint64_t)y*pitch+(uint64_t)region->x*bpp;
   if(n>sizeof linear || pos+n>available ||
      read(opaque,base+pos,linear,n)!=0)return PSP_TEXTURE_STREAM_IO_ERROR;
   result.source_reads++;result.source_bytes+=(uint32_t)n;
   for(size_t col=0;col<region->width;col++)
    pixel(linear+col*bpp,t->format,destination+col*4u);
  }else{
   const size_t block_cols=pitch/16u;
   for(size_t col=0;col<region->width;col++){
    const size_t x=(size_t)region->x+col;
    const size_t byte_x=x*bpp;
    const uint64_t block=((uint64_t)(y/8u)*block_cols+
                          byte_x/16u)*128u;
    if(block+128u>available)return PSP_TEXTURE_TRUNCATED;
    unsigned slot=PSP_TEXTURE_STREAM_SWIZZLE_SLOTS;
    for(unsigned j=0;j<PSP_TEXTURE_STREAM_SWIZZLE_SLOTS;j++)
     if(tiles[j].valid&&tiles[j].block==block){slot=j;break;}
    if(slot==PSP_TEXTURE_STREAM_SWIZZLE_SLOTS){
     slot=0u;
     for(unsigned j=0;j<PSP_TEXTURE_STREAM_SWIZZLE_SLOTS;j++){
      if(!tiles[j].valid){slot=j;break;}
      if(tiles[j].age<tiles[slot].age)slot=j;
     }
     tiles[slot].valid=0u;
     if(read(opaque,base+block,tiles[slot].bytes,
             PSP_TEXTURE_STREAM_SWIZZLE_BYTES)!=0)
      return PSP_TEXTURE_STREAM_IO_ERROR;
     tiles[slot].valid=1u;tiles[slot].block=block;
     result.source_reads++;
     result.source_bytes+=PSP_TEXTURE_STREAM_SWIZZLE_BYTES;
    }else result.cached_blocks++;
    tiles[slot].age=++tick;
    const size_t off=(y%8u)*16u+(byte_x%16u);
    pixel(tiles[slot].bytes+off,t->format,destination+col*4u);
   }
  }
 }
 result.decoded_pixels=(uint32_t)region->width*region->height;
 if(stats)*stats=result;
 return PSP_TEXTURE_OK;
}
