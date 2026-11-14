#include "psp_ge_subset.h"
#include "psp_hle.h"
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


static void guest_display_list_atomicity(void) {
 const uint8_t vertices[] = {
  0x1f, 0xfc, 3, 0, 4, 0, 0, 0,
  0x1f, 0xfc, 11, 0, 12, 0, 0, 0,
  0xe0, 0x83, 20, 0, 21, 0, 0, 0,
  0xe0, 0x83, 27, 0, 30, 0, 0, 0
 };
 uint32_t commands[] = {
  0x10080000u, 0x01000020u, 0x12800114u,
  SPRITE_CMD, SPRITE_CMD, FINISH, END
 };
 PspGeSubsetResult result;
 assert(sizeof vertices == 4u * 8u); /* four packed 8-byte vertices */
 assert(psp_ge_subset_translate_memory(commands, 7, vertices,
        sizeof vertices, 0x08000020u, &result) == PSP_GE_SUBSET_OK);
 assert(result.batch.count == 2 && result.consumed_sprites == 2);
 /* A bad second sprite must not expose the first as a partial frame. */
 uint8_t corrupt[sizeof vertices];
 memcpy(corrupt, vertices, sizeof corrupt);
 corrupt[25] ^= 1u; /* second sprite's endpoint color */
 memset(&result, 0xa5, sizeof result);
 assert(psp_ge_subset_translate_memory(commands, 7, corrupt,
        sizeof corrupt, 0x08000020u, &result) != PSP_GE_SUBSET_OK);
 assert(result.batch.count == 0 && result.consumed_sprites == 0);
 /* Commands after END, and state mutations after FINISH, are rejected. */
 uint32_t trailing[] = {
  0x10080000u, 0x01000020u, 0x12800114u,
  SPRITE_CMD, FINISH, END, 0
 };
 assert(psp_ge_subset_translate_memory(trailing, 7, vertices,
        sizeof vertices, 0x08000020u, &result) != PSP_GE_SUBSET_OK);
 assert(result.batch.count == 0);
 uint32_t late_state[] = {
  0x10080000u, 0x01000020u, 0x12800114u,
  SPRITE_CMD, FINISH, 0x01000020u, END
 };
 assert(psp_ge_subset_translate_memory(late_state, 7, vertices,
        sizeof vertices, 0x08000020u, &result) != PSP_GE_SUBSET_OK);
 assert(result.batch.count == 0);
}


static void guest_display_list_from_ram(void) {
 uint8_t memory[128] = {0};
 const uint8_t vertices[] = {
  0x1f, 0xfc, 3, 0, 4, 0, 0, 0,
  0x1f, 0xfc, 11, 0, 12, 0, 0, 0
 };
 const uint32_t words[] = {
  0x10080000u, 0x01000020u, 0x12800114u,
  SPRITE_CMD, FINISH, END
 };
 memcpy(memory + 0x20, vertices, sizeof vertices);
 for (size_t i = 0; i < sizeof words / sizeof words[0]; ++i) {
  uint32_t w = words[i];
  for (unsigned j = 0; j < 4; ++j)
   memory[0x60 + i * 4 + j] = (uint8_t)(w >> (j * 8));
 }
 PspGeSubsetResult result;
 assert(psp_ge_subset_translate_guest(memory, sizeof memory, 0x08000000u,
         0x08000060u, 6, &result) == PSP_GE_SUBSET_OK);
 assert(result.batch.count == 1 && result.batch.rects[0].w == 8);
 assert(result.consumed_commands == 6 && result.consumed_sprites == 1);
 PspGuestBus bus;
 assert(psp_guest_bus_init(&bus, memory, 0x08000000u, sizeof memory, NULL) == 0);
 PspGeSubsetResult bridged;
 assert(psp_ge_subset_translate_bus(&bus, 0x08000060u, 6, &bridged) == PSP_GE_SUBSET_OK);
 assert(memcmp(&result, &bridged, sizeof result) == 0);
 PspCpu cpu;
 psp_cpu_init(&cpu, memory, sizeof memory, 0x08000000u, 0x08000060u);
 cpu.gpr[4] = 0x08000060u;
 cpu.gpr[5] = 6;
 PspGeSubsetResult submitted;
 /* Execute real MIPS instructions to populate the submission registers. */
 cpu.gpr[4] = 0;
 cpu.gpr[5] = 0;
 assert(psp_guest_bus_write32(&bus, 0x08000000u, 0x3c040800u) == 0); /* lui a0, 0x0800 */
 assert(psp_guest_bus_write32(&bus, 0x08000004u, 0x34840060u) == 0); /* ori a0, a0, 0x60 */
 assert(psp_guest_bus_write32(&bus, 0x08000008u, 0x24050006u) == 0); /* addiu a1, zero, 6 */
 cpu.pc = 0x08000000u;
 cpu.next_pc = 0x08000004u;
 assert(psp_cpu_step(&cpu) == 0);
 assert(psp_cpu_step(&cpu) == 0);
 assert(psp_cpu_step(&cpu) == 0);
 assert(cpu.gpr[4] == 0x08000060u && cpu.gpr[5] == 6u);
 assert(psp_ge_subset_submit_cpu(&cpu, 4, 5, &submitted) == PSP_GE_SUBSET_OK);
 assert(memcmp(&result, &submitted, sizeof result) == 0);
 /* Complete emulated syscall -> HLE callback -> GE batch path. */
 PspHle hle;
 psp_hle_init(&hle);
 assert(psp_hle_bind(&hle, 42, PSP_HLE_GE_SUBMIT) == 0);
 /* Direct calls without a syscall trap must not publish a scene. */
 assert(psp_ge_subset_hle_submit(&cpu, &submitted) == -1);
 assert(submitted.batch.count == 0);
 assert(psp_ge_subset_hle_submit(&cpu, NULL) == -1);
 psp_hle_attach_ge(&hle, psp_ge_subset_hle_submit, &submitted);
 assert(psp_guest_bus_write32(&bus, 0x0800000cu, (42u << 6) | 12u) == 0);
 cpu.pc = 0x0800000cu;
 cpu.next_pc = 0x08000010u;
 assert(psp_hle_step(&hle, &cpu) == 1);
 assert(cpu.gpr[2] == 0);
 assert(memcmp(&result, &submitted, sizeof result) == 0);
 assert(cpu.pc == 0x08000010u && cpu.fault == 0);
 /* HLE must resume normal guest instructions after the GE submission. */
 assert(psp_guest_bus_write32(&bus, 0x08000010u, 0x24060007u) == 0);
 assert(psp_cpu_step(&cpu) == 0 && cpu.gpr[6] == 7u);
 cpu.gpr[4] = 0x08000078u;
 cpu.pc = 0x0800000cu;
 cpu.next_pc = 0x08000010u;
 assert(psp_hle_step(&hle, &cpu) == 1);
 assert(cpu.gpr[2] == UINT32_MAX && submitted.batch.count == 0);
 assert(cpu.pc == 0x08000010u && cpu.fault == 0);
 /* Failed submission must not poison subsequent valid guest calls. */
 cpu.gpr[4] = 0x08000060u;
 cpu.gpr[5] = 6u;
 cpu.pc = 0x0800000cu;
 cpu.next_pc = 0x08000010u;
 assert(psp_hle_step(&hle, &cpu) == 1);
 assert(cpu.gpr[2] == 0 && cpu.fault == 0);
 assert(memcmp(&result, &submitted, sizeof result) == 0);
 /* Reject an oversized guest command count without publishing stale data. */
 cpu.gpr[5] = PSP_GE_SUBSET_MAX_COMMANDS + 1u;
 cpu.pc = 0x0800000cu;
 cpu.next_pc = 0x08000010u;
 assert(psp_hle_step(&hle, &cpu) == 1);
 assert(cpu.gpr[2] == UINT32_MAX && submitted.batch.count == 0);
 cpu.gpr[4] = 0x08000078u;
 assert(psp_ge_subset_submit_cpu(&cpu, 4, 5, &submitted) != PSP_GE_SUBSET_OK);
 assert(submitted.batch.count == 0);
 cpu.gpr[4] = 0x08000060u;
 cpu.fault = 1;
 assert(psp_ge_subset_submit_cpu(&cpu, 4, 5, &submitted) == PSP_GE_SUBSET_BAD_ARGUMENT);
 assert(submitted.batch.count == 0);
 cpu.fault = 0;
 assert(psp_ge_subset_submit_cpu(&cpu, 32, 5, &submitted) == PSP_GE_SUBSET_BAD_ARGUMENT);
 assert(submitted.batch.count == 0);
 assert(psp_ge_subset_translate_bus(&bus, 0x08000078u, 6, &bridged) != PSP_GE_SUBSET_OK);
 assert(bridged.batch.count == 0);
 /* A cold guest region must never be treated as hot executable GE RAM. */
 volatile uint16_t cold_words[16] = {0};
 PspGuestWindow cold;
 assert(psp_guest_window_init(&cold, cold_words, 0x09000000u,
                              sizeof cold_words, 1, 1) == 0);
 PspGuestBus split_bus;
 assert(psp_guest_bus_init(&split_bus, memory, 0x08000000u,
                           sizeof memory, &cold) == 0);
 assert(psp_ge_subset_translate_bus(&split_bus, 0x08000060u, 6,
                                    &bridged) == PSP_GE_SUBSET_OK);
 assert(bridged.batch.count == 1);
 assert(psp_ge_subset_translate_bus(&split_bus, 0x09000000u, 6,
                                    &bridged) != PSP_GE_SUBSET_OK);
 assert(bridged.batch.count == 0);
 assert(psp_ge_subset_translate_guest(memory, sizeof memory, 0x08000000u,
         0x08000061u, 6, &result) != PSP_GE_SUBSET_OK);
 assert(result.batch.count == 0);
 assert(psp_ge_subset_translate_guest(memory, sizeof memory, 0x08000000u,
         0x08000078u, 6, &result) != PSP_GE_SUBSET_OK);
 assert(result.batch.count == 0);
 assert(psp_ge_subset_translate_guest(memory, sizeof memory, 0x08000000u,
         0x07fffffcu, 6, &result) != PSP_GE_SUBSET_OK);
 assert(result.batch.count == 0);
 assert(psp_ge_subset_translate_guest(memory, sizeof memory, 0x08000000u,
         0x08000060u, PSP_GE_SUBSET_MAX_COMMANDS + 1, &result) != PSP_GE_SUBSET_OK);
 assert(result.batch.count == 0);
 memory[0x60 + 3 * 4 + 3] = 0x08; /* unsupported opcode replaces PRIM */
 assert(psp_ge_subset_translate_guest(memory, sizeof memory, 0x08000000u,
         0x08000060u, 6, &result) == PSP_GE_SUBSET_UNSUPPORTED);
 assert(result.batch.count == 0);
}

static void guest_multi_sprite_list(void) {
 uint8_t ram[192] = {0};
 const uint32_t base = 0x08000000u;
 /* Three 8-byte vertex pairs, directly stored in PSP guest memory. */
 const uint16_t boxes[3][5] = {
  {3,4,11,12,0xfc1fu},
  {20,25,26,32,0x83e0u},
  {40,50,49,60,0x801fu}
 };
 for(size_t i=0;i<3;i++){
  size_t off=0x20u+i*16u;
  uint16_t color=boxes[i][4];
  for(unsigned v=0;v<2;v++){
   uint16_t x=boxes[i][v?2:0], y=boxes[i][v?3:1];
   size_t p=off+v*8u;
   ram[p]=(uint8_t)color;ram[p+1]=(uint8_t)(color>>8);
   ram[p+2]=(uint8_t)x;ram[p+3]=(uint8_t)(x>>8);
   ram[p+4]=(uint8_t)y;ram[p+5]=(uint8_t)(y>>8);
  }
 }
 const uint32_t commands[]={
  0x10080000u,0x01000020u,0x12800114u,
  SPRITE_CMD,SPRITE_CMD,SPRITE_CMD,FINISH,END
 };
 for(size_t i=0;i<sizeof commands/sizeof commands[0];i++)
  for(unsigned b=0;b<4;b++)
   ram[0x80u+i*4u+b]=(uint8_t)(commands[i]>>(8u*b));
 PspGeSubsetResult output;
 assert(psp_ge_subset_translate_guest(ram,sizeof ram,base,base+0x80u,8,&output)
        == PSP_GE_SUBSET_OK);
 assert(output.batch.count==3 && output.consumed_sprites==3);
 assert(output.consumed_commands==8);
 for(size_t i=0;i<3;i++){
  assert(output.batch.rects[i].x==boxes[i][0]);
  assert(output.batch.rects[i].y==boxes[i][1]);
  assert(output.batch.rects[i].w==boxes[i][2]-boxes[i][0]);
  assert(output.batch.rects[i].h==boxes[i][3]-boxes[i][1]);
  assert(output.batch.rects[i].bgr555==(boxes[i][4]&0x7fffu));
 }
 PspGuestBus bus;
 assert(psp_guest_bus_init(&bus,ram,base,sizeof ram,NULL)==0);
 PspGeSubsetResult via_bus;
 assert(psp_ge_subset_translate_bus(&bus,base+0x80u,8,&via_bus)
        == PSP_GE_SUBSET_OK);
 assert(memcmp(&output,&via_bus,sizeof output)==0);
 /* Third primitive is malformed; the entire scene must be discarded. */
 ram[0x20u+2u*16u+8u+2u]=40u;
 assert(psp_ge_subset_translate_guest(ram,sizeof ram,base,base+0x80u,8,&output)
        == PSP_GE_SUBSET_MALFORMED);
 assert(output.batch.count==0 && output.consumed_sprites==0);
}

int main(void){
 guest_multi_sprite_list();
 guest_display_list_from_ram();
 guest_display_list_atomicity();
 guest_memory_rejections();
 guest_memory();
 for(uint32_t frame=0;frame<8;frame++)raster_match(frame*11u);
 /* A single GE list can submit multiple sprite primitives in order. */
 const PspGeDecodedSprite multi_sprites[]={
  {3,4,8,8,0x7c1f},{20,25,6,7,0x03e0},{40,50,9,10,0x001f}
 };
 const uint32_t multi_commands[]={
  SPRITE_CMD,SPRITE_CMD,SPRITE_CMD,FINISH,END
 };
 PspGeSubsetResult multi;
 assert(psp_ge_subset_translate(multi_commands,5,multi_sprites,3,&multi)
        == PSP_GE_SUBSET_OK);
 assert(multi.batch.count==3 && multi.consumed_sprites==3);
 assert(multi.consumed_commands==5);
 for(size_t i=0;i<3;i++){
  assert(multi.batch.rects[i].x==multi_sprites[i].x);
  assert(multi.batch.rects[i].y==multi_sprites[i].y);
  assert(multi.batch.rects[i].w==multi_sprites[i].w);
  assert(multi.batch.rects[i].h==multi_sprites[i].h);
  assert(multi.batch.rects[i].bgr555==multi_sprites[i].bgr555);
 }
 assert(psp_ge_subset_translate(multi_commands,5,multi_sprites,2,&multi)
        != PSP_GE_SUBSET_OK);
 assert(multi.batch.count==0 && multi.consumed_sprites==0);
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
