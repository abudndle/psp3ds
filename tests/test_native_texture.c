#include "psp_native_texture.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static void pixel(const uint8_t *p,unsigned r,unsigned g,unsigned b,unsigned a) {
 assert(p[0]==r&&p[1]==g&&p[2]==b&&p[3]==a);
}
static void formats(void) {
 const PspNativeTextureRegion area={0,0,2,2};
 PspNativeTexture t={2,2,2,PSP_TEXTURE_5650,0};
 uint8_t out[16]={0};
 const uint8_t rgb565[]={0x1f,0,0xe0,0x07,0x00,0xf8,0xff,0xff};
 assert(psp_native_texture_decode_region(&t,rgb565,sizeof rgb565,&area,
                                          out,8,sizeof out)==0);
 pixel(out,255,0,0,255); pixel(out+4,0,255,0,255);
 pixel(out+8,0,0,255,255); pixel(out+12,255,255,255,255);
 t.format=PSP_TEXTURE_5551;
 const uint8_t rgb5551[]={0x1f,0x80,0x1f,0x00,0xe0,0x83,0xff,0xff};
 assert(psp_native_texture_decode_region(&t,rgb5551,sizeof rgb5551,&area,
                                          out,8,sizeof out)==0);
 pixel(out,255,0,0,255); pixel(out+4,255,0,0,0);
 pixel(out+8,0,255,0,255); pixel(out+12,255,255,255,255);
 t.format=PSP_TEXTURE_4444;
 const uint8_t rgb4444[]={0x0f,0xf0,0xf0,0x80,0x00,0x0f,0xff,0xff};
 assert(psp_native_texture_decode_region(&t,rgb4444,sizeof rgb4444,&area,
                                          out,8,sizeof out)==0);
 pixel(out,255,0,0,255);pixel(out+4,0,255,0,136);
 pixel(out+8,0,0,255,0); pixel(out+12,255,255,255,255);
 t.format=PSP_TEXTURE_8888;
 const uint8_t rgba[]={1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16};
 assert(psp_native_texture_decode_region(&t,rgba,sizeof rgba,&area,
                                          out,8,sizeof out)==0);
 assert(memcmp(out,rgba,sizeof out)==0);
}
static void crop_stride(void) {
 PspNativeTexture t={2,2,4,PSP_TEXTURE_5650,0};
 PspNativeTextureRegion area={1,0,1,2};
 const uint8_t data[]={
  0x1f,0,0xe0,0x07,0,0,0,0,
  0x00,0xf8,0xff,0xff,0,0,0,0
 };
 uint8_t out[13];memset(out,0xa5,sizeof out);
 assert(psp_native_texture_decode_region(&t,data,sizeof data,&area,
                                          out,9,sizeof out)==0);
 pixel(out,0,255,0,255);
 pixel(out+9,255,255,255,255);
 assert(out[4]==0xa5&&out[8]==0xa5);
 assert(psp_native_texture_decode_region(&t,data,sizeof data-1,&area,
                                          out,9,sizeof out)==PSP_TEXTURE_TRUNCATED);
 assert(psp_native_texture_decode_region(&t,data,sizeof data,&area,
                                          out,9,sizeof out-1)==PSP_TEXTURE_TRUNCATED);
}
static void swizzle(void) {
 /* 16x16 5650: four 16-byte x 8-row blocks in memory. */
 uint8_t blocks[512];
 const unsigned colors[]={0x001f,0x07e0,0xf800,0xffff};
 for(unsigned block=0;block<4;block++)
  for(unsigned i=0;i<64;i++) {
   blocks[block*128+i*2]=(uint8_t)colors[block];
   blocks[block*128+i*2+1]=(uint8_t)(colors[block]>>8);
  }
 PspNativeTexture t={16,16,16,PSP_TEXTURE_5650,1};
 PspNativeTextureRegion area={7,7,2,2};
 uint8_t out[16];
 assert(psp_native_texture_decode_region(&t,blocks,sizeof blocks,&area,
                                          out,8,sizeof out)==0);
 pixel(out,255,0,0,255);
 pixel(out+4,0,255,0,255);
 pixel(out+8,0,0,255,255);
 pixel(out+12,255,255,255,255);
 assert(psp_native_texture_decode_region(&t,blocks,sizeof blocks-1,&area,
                                          out,8,sizeof out)==PSP_TEXTURE_TRUNCATED);

 /* 8888: the two 128-byte blocks split at x=4, not x=8. */
 uint8_t wide[256];
 for(unsigned i=0;i<32;i++) {
  wide[i*4+0]=1;wide[i*4+1]=2;wide[i*4+2]=3;wide[i*4+3]=4;
  wide[128+i*4+0]=5;wide[128+i*4+1]=6;
  wide[128+i*4+2]=7;wide[128+i*4+3]=8;
 }
 t=(PspNativeTexture){8,8,8,PSP_TEXTURE_8888,1};
 area=(PspNativeTextureRegion){3,0,2,1};
 assert(psp_native_texture_decode_region(&t,wide,sizeof wide,&area,
                                          out,8,sizeof out)==0);
 pixel(out,1,2,3,4);pixel(out+4,5,6,7,8);

 /* Last partial swizzled row is stored in a complete 8-row block. */
 uint8_t partial[256]={0};
 partial[128]=0x1f;partial[129]=0;
 t=(PspNativeTexture){8,9,8,PSP_TEXTURE_5650,1};
 area=(PspNativeTextureRegion){0,8,1,1};
 assert(psp_native_texture_decode_region(&t,partial,sizeof partial,&area,
                                          out,4,sizeof out)==0);
 pixel(out,255,0,0,255);
 assert(psp_native_texture_decode_region(&t,partial,sizeof partial-1,&area,
                                          out,4,sizeof out)==PSP_TEXTURE_TRUNCATED);
}
static void invalid(void) {
 uint8_t source[256]={0};
 uint8_t out[128];memset(out,0xee,sizeof out);
 PspNativeTexture t={8,8,8,PSP_TEXTURE_5650,1};
 PspNativeTextureRegion r={0,0,2,2};
 assert(psp_native_texture_decode_region(NULL,source,sizeof source,&r,
                                          out,8,sizeof out)==PSP_TEXTURE_BAD_ARGUMENT);
 assert(psp_native_texture_decode_region(&t,NULL,sizeof source,&r,
                                          out,8,sizeof out)==PSP_TEXTURE_BAD_ARGUMENT);
 assert(psp_native_texture_decode_region(&t,source,sizeof source,NULL,
                                          out,8,sizeof out)==PSP_TEXTURE_BAD_ARGUMENT);
 assert(psp_native_texture_decode_region(&t,source,sizeof source,&r,
                                          NULL,8,sizeof out)==PSP_TEXTURE_BAD_ARGUMENT);
 t.format=PSP_TEXTURE_5650+4; /* unsupported CLUT4 must fail closed */
 assert(psp_native_texture_decode_region(&t,source,sizeof source,&r,
                                          out,8,sizeof out)==PSP_TEXTURE_UNSUPPORTED);
 t=(PspNativeTexture){8,8,9,PSP_TEXTURE_5650,1};
 assert(psp_native_texture_decode_region(&t,source,sizeof source,&r,
                                          out,8,sizeof out)==PSP_TEXTURE_BAD_ARGUMENT);
 t=(PspNativeTexture){8,8,8,PSP_TEXTURE_5650,2};
 assert(psp_native_texture_decode_region(&t,source,sizeof source,&r,
                                          out,8,sizeof out)==PSP_TEXTURE_BAD_ARGUMENT);
 t=(PspNativeTexture){8,8,8,PSP_TEXTURE_5650,1};
 r=(PspNativeTextureRegion){7,7,2,1};
 assert(psp_native_texture_decode_region(&t,source,sizeof source,&r,
                                          out,8,sizeof out)==PSP_TEXTURE_BAD_ARGUMENT);
 r=(PspNativeTextureRegion){0,0,2,2};
 assert(psp_native_texture_decode_region(&t,source,sizeof source,&r,
                                          out,7,sizeof out)==PSP_TEXTURE_TRUNCATED);
 assert(psp_native_texture_decode_region(&t,source,sizeof source,&r,
                                          source,8,sizeof source)==PSP_TEXTURE_BAD_ARGUMENT);
 for(unsigned i=0;i<sizeof out;i++)assert(out[i]==0xee);
}
int main(void) {
 formats(); crop_stride(); swizzle(); invalid();
 puts("PASS: portable PSP GE direct-color textures, swizzle, small-region decode, strict bounds");
 return 0;
}
