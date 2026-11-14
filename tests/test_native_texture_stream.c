#include "psp_native_texture_stream.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#define LARGE_OFFSET UINT64_C(0x100001000)
typedef struct {
 const uint8_t *bytes;
 size_t size;
 uint64_t origin;
 unsigned calls;
 unsigned fail_on;
} Input;
static int read_at(void *opaque,uint64_t at,uint8_t *dst,size_t n){
 Input *src=(Input*)opaque;
 if(!src||!dst||at<src->origin||at-src->origin>src->size||
    n>src->size-(size_t)(at-src->origin))return -1;
 src->calls++;
 if(src->fail_on && src->calls==src->fail_on)return -1;
 memcpy(dst,src->bytes+(size_t)(at-src->origin),n);
 return 0;
}
static void compare_all_formats(void){
 static uint8_t pixels[64*64*4];
 static uint8_t wanted[8*8*4],received[8*8*4];
 for(unsigned i=0;i<sizeof pixels;i++)pixels[i]=(uint8_t)(i*17u+3u);
 for(unsigned format=0;format<4;format++){
  PspNativeTexture texture={64,64,64,(uint8_t)format,0};
  PspNativeTextureRegion region={3,5,8,8};
  const size_t bpp=format==PSP_TEXTURE_8888?4u:2u;
  const size_t len=64u*64u*bpp;
  Input input={pixels,len,LARGE_OFFSET,0,0};
  memset(wanted,0x5a,sizeof wanted);
  memset(received,0xe5,sizeof received);
  assert(psp_native_texture_decode_region(&texture,pixels,len,&region,
                                          wanted,32,sizeof wanted)==0);
  PspTextureStreamStats stats;
  assert(psp_native_texture_decode_stream(&texture,LARGE_OFFSET,len,
           &region,read_at,&input,received,32,sizeof received,&stats)==0);
  assert(memcmp(wanted,received,sizeof wanted)==0);
  assert(input.calls==8u && stats.source_reads==8u);
  assert(stats.source_bytes==8u*8u*bpp);
  assert(stats.decoded_pixels==64u);
  assert(stats.source_bytes<len);
 }
}
static void swizzled_block_cache(void){
 uint8_t blocks[512];
 const unsigned colors[]={0x001f,0x07e0,0xf800,0xffff};
 for(unsigned tile=0;tile<4;tile++)
  for(unsigned i=0;i<64;i++){
   blocks[tile*128+i*2u]=(uint8_t)colors[tile];
   blocks[tile*128+i*2u+1u]=(uint8_t)(colors[tile]>>8);
  }
 PspNativeTexture texture={16,16,16,PSP_TEXTURE_5650,1};
 PspNativeTextureRegion region={7,7,2,2};
 uint8_t old[16],new_output[16];
 Input input={blocks,sizeof blocks,123u,0,0};
 assert(psp_native_texture_decode_region(&texture,blocks,sizeof blocks,
                   &region,old,8,sizeof old)==0);
 PspTextureStreamStats stats;
 assert(psp_native_texture_decode_stream(&texture,123u,sizeof blocks,
                   &region,read_at,&input,new_output,8,sizeof new_output,
                   &stats)==0);
 assert(memcmp(old,new_output,sizeof old)==0);
 assert(stats.source_reads==4u);
 assert(stats.decoded_pixels==4u);

 /* Full 8x8 tile lies in one PSP swizzle block: only 128 bytes read,
  * no 512-byte whole source texture allocation. */
 region=(PspNativeTextureRegion){0,0,8,8};
 uint8_t ref[8u*8u*4u],out[8u*8u*4u];
 input.calls=0;
 assert(psp_native_texture_decode_region(&texture,blocks,sizeof blocks,
                    &region,ref,32,sizeof ref)==0);
 assert(psp_native_texture_decode_stream(&texture,123u,sizeof blocks,
                    &region,read_at,&input,out,32,sizeof out,&stats)==0);
 assert(memcmp(ref,out,sizeof ref)==0);
 assert(input.calls==1u && stats.source_reads==1u &&
        stats.source_bytes==128u &&
        stats.cached_blocks==63u && stats.decoded_pixels==64u);
}
static void high_memory_avoidance(void){
 /* Large host fixture remains in host test only: native decoder has fixed
  * 2KiB scanline and 4 swizzle blocks, no full-texture copy. */
 static uint8_t source[512u*512u*4u];
 for(unsigned i=0;i<sizeof source;i++)source[i]=(uint8_t)(i*7u+1u);
 PspNativeTexture tex={512,512,512,PSP_TEXTURE_8888,0};
 PspNativeTextureRegion rect={301,412,8,8};
 uint8_t direct[256],streamed[256];
 Input input={source,sizeof source,LARGE_OFFSET,0,0};
 PspTextureStreamStats stats;
 assert(psp_native_texture_decode_region(&tex,source,sizeof source,&rect,
                                        direct,32,sizeof direct)==0);
 assert(psp_native_texture_decode_stream(&tex,LARGE_OFFSET,
             sizeof source,&rect,read_at,&input,streamed,32,
             sizeof streamed,&stats)==0);
 assert(memcmp(direct,streamed,sizeof direct)==0);
 assert(stats.source_bytes==256u &&
        stats.source_reads==8u &&
        stats.source_bytes*4096u==sizeof source);
}
static void invalid_and_io_fault(void){
 uint8_t data[256]={0},out[32];
 PspNativeTexture tex={8,8,8,PSP_TEXTURE_5650,1};
 PspNativeTextureRegion rect={0,0,2,2};
 Input input={data,sizeof data,0u,0,0};
 PspTextureStreamStats stats;
 memset(out,0x5a,sizeof out);
 assert(psp_native_texture_decode_stream(NULL,0,256,&rect,read_at,
             &input,out,8,sizeof out,&stats)==PSP_TEXTURE_BAD_ARGUMENT);
 assert(stats.decoded_pixels==0u);
 assert(psp_native_texture_decode_stream(&tex,0,255,&rect,read_at,
             &input,out,8,sizeof out,&stats)==PSP_TEXTURE_TRUNCATED);
 assert(psp_native_texture_decode_stream(&tex,UINT64_MAX-8u,256u,
             &rect,read_at,&input,out,8,sizeof out,&stats)==PSP_TEXTURE_TRUNCATED);
 input.fail_on=1;
 assert(psp_native_texture_decode_stream(&tex,0,256,&rect,read_at,
             &input,out,8,sizeof out,&stats)==PSP_TEXTURE_STREAM_IO_ERROR);
 assert(stats.decoded_pixels==0u);
 for(unsigned i=0;i<sizeof out;i++)assert(out[i]==0x5au);
 input.fail_on=0;
 tex.format=4u;
 assert(psp_native_texture_decode_stream(&tex,0,256,&rect,read_at,
             &input,out,8,sizeof out,&stats)==PSP_TEXTURE_UNSUPPORTED);
}
int main(void){
 compare_all_formats();swizzled_block_cache();
 high_memory_avoidance();invalid_and_io_fault();
 puts("PASS: on-demand PSP direct-color tiles with bounded 2.6KiB scratch and no full texture in guest heap");
 return 0;
}
