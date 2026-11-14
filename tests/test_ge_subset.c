#include "psp_ge_subset.h"
#include "psp_scene.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#define SPRITE_CMD UINT32_C(0x04060002)
#define FINISH UINT32_C(0x0f000000)
#define END UINT32_C(0x0c000000)
static void raster_match(uint32_t frame){
 PspScene s;
 PspSceneBatch original;
 PspGeSubsetResult translated;
 PspGeDecodedSprite vertices[PSP_SCENE_MAX_RECTS];
 uint32_t commands[PSP_GE_SUBSET_MAX_COMMANDS];
 assert(psp_scene_build(&s,frame)==0);
 assert(psp_scene_batch_build(&s,&original)==0);
 size_t n=original.count;
 assert(n>0&&n+2<=PSP_GE_SUBSET_MAX_COMMANDS);
 for(size_t i=0;i<n;i++){
  const PspSceneRect *r=&original.rects[i];
  vertices[i]=(PspGeDecodedSprite){r->x,r->y,r->w,r->h,r->bgr555};
  commands[i]=SPRITE_CMD;
 }
 commands[n]=FINISH;commands[n+1]=END;
 assert(psp_ge_subset_translate(commands,n+2,vertices,n,&translated)==0);
 assert(translated.consumed_commands==n+2);
 assert(translated.consumed_sprites==n);
 assert(translated.batch.count==n);
 assert(memcmp(&translated.batch,&original,sizeof original)==0);
 for(unsigned y=0;y<192;y++)for(unsigned x=0;x<256;x++){
  uint16_t a=0,b=0;
  assert(psp_scene_color(&s,x,y,&a)==0);
  assert(psp_scene_batch_color(&translated.batch,x,y,&b)==0);
  assert(a==b);
 }
}
static void guest_memory(void) {
 const uint8_t vertices[] = {
  0x1f, 0xfc, 3, 0, 4, 0, 0, 0, 0x1f, 0xfc, 11, 0, 12, 0, 0, 0,
  0xe0, 0x83, 20, 0, 21, 0, 0, 0, 0xe0, 0x83, 27, 0, 30, 0, 0, 0
 };
 uint32_t words[] = {0x10080000u, 0x01000020u, 0x12800114u,
                     SPRITE_CMD, SPRITE_CMD, FINISH, END};
 PspGeSubsetResult result;
 assert(psp_ge_subset_translate_memory(words, 7, vertices, sizeof vertices,
                                       0x08000020u, &result) == 0);
 assert(result.batch.count == 2 && result.consumed_commands == 7);
 assert(result.batch.rects[0].x == 3 && result.batch.rects[0].w == 8);
 assert(result.batch.rects[0].bgr555 == 0x7c1f);
 assert(result.batch.rects[1].y == 21 && result.batch.rects[1].h == 9);
 assert(result.batch.rects[1].bgr555 == 0x03e0);
 assert(psp_ge_subset_translate_memory(words, 7, vertices, sizeof vertices - 1,
                                       0x08000020u, &result) < 0);
 assert(result.batch.count == 0 && result.consumed_sprites == 0);
 words[1] = 0x01000021u;
 assert(psp_ge_subset_translate_memory(words, 7, vertices, sizeof vertices,
                                       0x08000020u, &result) < 0);
 words[1] = 0x01000020u;
 words[2] = 0x12800115u;
 assert(psp_ge_subset_translate_memory(words, 7, vertices, sizeof vertices,
                                       0x08000020u, &result) ==
        PSP_GE_SUBSET_UNSUPPORTED);
 assert(result.batch.count == 0);
}


static void guest_memory_rejections(void) {
 const uint8_t valid[] = {
  0x1f, 0xfc, 3, 0, 4, 0, 0, 0,
  0x1f, 0xfc, 11, 0, 12, 0, 0, 0
 };
 const uint32_t baseline[] = {
  0x10080000u, 0x01000020u, 0x12800114u,
  SPRITE_CMD, FINISH, END
 };
 uint8_t bytes[sizeof valid];
 uint32_t commands[sizeof baseline / sizeof baseline[0]];
 PspGeSubsetResult result;
 /* Each failure must clear all output, even if the caller reuses it. */
 for (unsigned scenario = 0; scenario < 10; ++scenario) {
  memcpy(bytes, valid, sizeof bytes);
  memcpy(commands, baseline, sizeof baseline);
  size_t size = sizeof bytes;
  uint32_t memory_base = 0x08000020u;
  switch (scenario) {
   case 0: bytes[1] &= 0x7fu; break; /* non-opaque vertex */
   case 1: bytes[8] ^= 1u; break; /* mismatched endpoint color */
   case 2: bytes[14] = 1; break; /* mismatched depth */
   case 3: bytes[10] = 3; break; /* zero-width rectangle */
   case 4: bytes[12] = 4; break; /* zero-height rectangle */
   case 5: bytes[10] = 0xff; bytes[11] = 1; break; /* clipped right */
   case 6: bytes[12] = 193; break; /* clipped bottom */
   case 7: size = 15; break; /* incomplete endpoint */
   case 8: commands[1] = 0x0100001eu; break; /* address before window */
   case 9: commands[2] = 0; break; /* missing vertex format */
  }
  memset(&result, 0xa5, sizeof result);
  assert(psp_ge_subset_translate_memory(commands, 6, bytes, size,
                                         memory_base, &result) != PSP_GE_SUBSET_OK);
  assert(result.batch.count == 0);
  assert(result.consumed_sprites == 0);
  assert(result.consumed_commands == 0);
 }
 /* A valid command stream must still succeed after rejected inputs. */
 assert(psp_ge_subset_translate_memory(baseline, 6, valid, sizeof valid,
                                       0x08000020u, &result) == PSP_GE_SUBSET_OK);
 assert(result.batch.count == 1 && result.consumed_sprites == 1);
}

int main(void){
 guest_memory_rejections();
 guest_memory();
 for(uint32_t frame=0;frame<8;frame++)raster_match(frame*11u);
 const PspGeDecodedSprite sprite={3,4,8,8,0x7c1f};
 PspGeSubsetResult result;
 uint32_t good[]={0,SPRITE_CMD,FINISH,END};
 assert(psp_ge_subset_translate(good,4,&sprite,1,&result)==0);
 assert(result.batch.count==1);
 uint32_t bad[][4]={
  {0x12000000,SPRITE_CMD,FINISH,END}, /* VTYPE requires real decoding */
  {0x01000000,SPRITE_CMD,FINISH,END}, /* VADDR unsupported */
  {0x10000000,SPRITE_CMD,FINISH,END}, /* BASE unsupported */
  {0x08000000,SPRITE_CMD,FINISH,END}, /* JUMP unsupported */
  {0,0x04030002,FINISH,END},          /* triangles not sprites */
  {0,0x04060003,FINISH,END},          /* wrong vertex count */
  {0x00000001,SPRITE_CMD,FINISH,END}, /* nonzero NOP */
  {0,SPRITE_CMD,END,FINISH},          /* wrong terminator order */
  {0,SPRITE_CMD,FINISH,0},            /* missing END */
  {0,SPRITE_CMD,FINISH,0x0c000001}   /* unsupported END arg */
 };
 for(size_t i=0;i<sizeof bad/sizeof bad[0];i++){
  memset(&result,0xa5,sizeof result);
  assert(psp_ge_subset_translate(bad[i],4,&sprite,1,&result)!=0);
  assert(result.batch.count==0&&result.consumed_sprites==0);
 }
 assert(psp_ge_subset_translate(good,4,&sprite,0,&result)!=0);
 assert(psp_ge_subset_translate(good,4,NULL,1,&result)!=0);
 assert(psp_ge_subset_translate(good,4,&sprite,1,NULL)!=0);
 assert(psp_ge_subset_translate(good,0,&sprite,1,&result)!=0);
 uint32_t too_long[PSP_GE_SUBSET_MAX_COMMANDS+1]={0};
 assert(psp_ge_subset_translate(too_long,PSP_GE_SUBSET_MAX_COMMANDS+1,&sprite,1,&result)!=0);
 PspGeDecodedSprite clipped={255,191,2,2,0x7c1f};
 assert(psp_ge_subset_translate(good,4,&clipped,1,&result)!=0);
 assert(result.batch.count==0);
 puts("PASS: limited PSP GE sprite opcode subset, 8 scene pixel parities, fail-closed unsupported state");
 return 0;
}
