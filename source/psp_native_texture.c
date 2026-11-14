#include "psp_native_texture.h"
#include <stdint.h>

static uint8_t expand5(unsigned v) { return (uint8_t)((v << 3) | (v >> 2)); }
static uint8_t expand6(unsigned v) { return (uint8_t)((v << 2) | (v >> 4)); }
static uint8_t expand4(unsigned v) { return (uint8_t)((v << 4) | v); }

int psp_native_texture_decode_region(
 const PspNativeTexture *texture,
 const uint8_t *source, size_t source_size,
 const PspNativeTextureRegion *region,
 uint8_t *rgba, size_t rgba_stride, size_t rgba_size) {
 if (!texture || !source || !region || !rgba)
  return PSP_TEXTURE_BAD_ARGUMENT;
 if (texture->format > PSP_TEXTURE_8888)
  return PSP_TEXTURE_UNSUPPORTED;
 if (texture->swizzled > 1 || !texture->width || !texture->height ||
     texture->width > 512u || texture->height > 512u ||
     texture->buffer_width < texture->width ||
     texture->buffer_width > 2048u || !region->width || !region->height ||
     region->x >= texture->width || region->y >= texture->height ||
     (uint32_t)region->x + region->width > texture->width ||
     (uint32_t)region->y + region->height > texture->height)
  return PSP_TEXTURE_BAD_ARGUMENT;

 const size_t bpp=texture->format==PSP_TEXTURE_8888?4u:2u;
 const size_t pitch=(size_t)texture->buffer_width*bpp;
 if (texture->swizzled && (pitch % 16u != 0u))
  return PSP_TEXTURE_BAD_ARGUMENT;
 const size_t source_rows=texture->swizzled?
   (((size_t)texture->height + 7u)/8u)*8u: texture->height;
 const size_t required=pitch*source_rows; /* max 2048*4*512 */
 if (source_size < required)
  return PSP_TEXTURE_TRUNCATED;
 const size_t line=(size_t)region->width*4u;
 if (rgba_stride < line || (region->height > 1u &&
     rgba_stride > (SIZE_MAX-line)/(region->height-1u)) ||
     rgba_size < (size_t)(region->height-1u)*rgba_stride+line)
  return PSP_TEXTURE_TRUNCATED;

 /* Fail closed on aliased source/destination so decoding cannot overwrite
  * texels that are still needed by a later region row. */
 const uintptr_t s=(uintptr_t)source;
 const uintptr_t d=(uintptr_t)rgba;
 const size_t dst_used=(size_t)(region->height-1u)*rgba_stride+line;
 if (s > UINTPTR_MAX-required || d > UINTPTR_MAX-dst_used ||
     (s < d+dst_used && d < s+required))
  return PSP_TEXTURE_BAD_ARGUMENT;

 for (size_t row=0; row<region->height; ++row) {
  const size_t y=(size_t)region->y+row;
  uint8_t *dst=rgba+row*rgba_stride;
  for (size_t col=0; col<region->width; ++col) {
   const size_t x=(size_t)region->x+col;
   size_t pos;
   if (texture->swizzled) {
    const size_t byte_x=x*bpp;
    const size_t block_cols=pitch/16u;
    pos=((y/8u)*block_cols + (byte_x/16u))*128u +
        (y%8u)*16u + (byte_x%16u);
   } else pos=y*pitch+x*bpp;
   uint8_t *pixel=dst+col*4u;
   if (texture->format==PSP_TEXTURE_8888) {
    pixel[0]=source[pos+0]; pixel[1]=source[pos+1];
    pixel[2]=source[pos+2]; pixel[3]=source[pos+3];
    continue;
   }
   const unsigned word=(unsigned)source[pos] |
                       ((unsigned)source[pos+1] << 8);
   if (texture->format==PSP_TEXTURE_5650) {
    pixel[0]=expand5(word & 31u);
    pixel[1]=expand6((word >> 5) & 63u);
    pixel[2]=expand5((word >> 11) & 31u);
    pixel[3]=255;
   } else if (texture->format==PSP_TEXTURE_5551) {
    pixel[0]=expand5(word & 31u);
    pixel[1]=expand5((word >> 5) & 31u);
    pixel[2]=expand5((word >> 10) & 31u);
    pixel[3]=(word & 0x8000u)?255:0;
   } else {
    pixel[0]=expand4(word & 15u);
    pixel[1]=expand4((word >> 4) & 15u);
    pixel[2]=expand4((word >> 8) & 15u);
    pixel[3]=expand4((word >> 12) & 15u);
   }
  }
 }
 return PSP_TEXTURE_OK;
}
