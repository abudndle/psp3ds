#include "psp_ge_subset.h"
#include <string.h>
static uint16_t read_le16(const uint8_t *bytes) {
 return (uint16_t)(bytes[0] | ((uint16_t)bytes[1] << 8));
}

/* Color 5551 followed by through-mode 16-bit X, Y and Z. */
static int read_sprite(const uint8_t *memory, size_t size, uint32_t memory_base,
                       uint32_t address, PspGeDecodedSprite *sprite) {
 if ((address & 1u) || address < memory_base || address > 0x0ffffff0u)
  return PSP_GE_SUBSET_MALFORMED;
 size_t offset = address - memory_base;
 if (offset > size || size - offset < 16u)
  return PSP_GE_SUBSET_MALFORMED;

 const uint8_t *first = memory + offset;
 const uint8_t *second = first + 8;
 uint16_t color = read_le16(first);
 uint16_t x = read_le16(first + 2);
 uint16_t y = read_le16(first + 4);
 uint16_t right = read_le16(second + 2);
 uint16_t bottom = read_le16(second + 4);
 if (!(color & 0x8000u) || color != read_le16(second) ||
     read_le16(first + 6) != read_le16(second + 6))
  return PSP_GE_SUBSET_UNSUPPORTED;
 if (right <= x || bottom <= y)
  return PSP_GE_SUBSET_MALFORMED;
 *sprite = (PspGeDecodedSprite){x, y, right - x, bottom - y, color & 0x7fffu};
 return PSP_GE_SUBSET_OK;
}

static int translate(const uint32_t *words, size_t count,
                     const PspGeDecodedSprite *sprites, size_t sprite_count,
                     const uint8_t *memory, size_t size, uint32_t memory_base,
                     PspGeSubsetResult *out) {
 uint32_t base = 0, address = 0;
 int has_format = 0, has_address = 0;
 if(out)memset(out,0,sizeof(*out));
 if(!out||!words||!count||count>PSP_GE_SUBSET_MAX_COMMANDS||
    sprite_count>PSP_GE_SUBSET_MAX_SPRITES||
    (sprite_count&&!sprites))return PSP_GE_SUBSET_BAD_ARGUMENT;
 PspGeSubsetResult staged;
 memset(&staged,0,sizeof staged);
 int finished=0,ended=0;
 size_t used=0;
 for(size_t i=0;i<count;i++){
  uint32_t word=words[i];
  unsigned opcode=word>>24;
  uint32_t arg=word&UINT32_C(0x00ffffff);
  if(ended)return PSP_GE_SUBSET_MALFORMED;
  if(opcode==0x00){ /* Only exact NOP with zero argument. */
   if(arg||finished)return PSP_GE_SUBSET_UNSUPPORTED;
  }else if(opcode==0x04){ /* PRIM: only sprite with two vertices. */
   if(finished)return PSP_GE_SUBSET_MALFORMED;
   if(arg!=UINT32_C(0x00060002))return PSP_GE_SUBSET_UNSUPPORTED;
   if (used >= PSP_GE_SUBSET_MAX_SPRITES) return PSP_GE_SUBSET_OVERFLOW;
   PspGeDecodedSprite decoded;
   if (memory) {
    if (!has_format || !has_address) return PSP_GE_SUBSET_MALFORMED;
    int status = read_sprite(memory, size, memory_base, address, &decoded);
    if (status != PSP_GE_SUBSET_OK) return status;
    address += 16u;
   } else {
    if (used >= sprite_count) return PSP_GE_SUBSET_MALFORMED;
    decoded = sprites[used];
   }
   ++used;
   const PspGeDecodedSprite *s = &decoded;
   if(!s->w||!s->h||s->x>=256u||s->y>=192u||
      (uint32_t)s->x+s->w>256u||(uint32_t)s->y+s->h>192u||
      s->bgr555>UINT16_C(0x7fff))return PSP_GE_SUBSET_MALFORMED;
   if(staged.batch.count>=PSP_SCENE_MAX_RECTS)return PSP_GE_SUBSET_OVERFLOW;
   PspSceneRect *r=&staged.batch.rects[staged.batch.count++];
   r->x=s->x;r->y=s->y;r->w=s->w;r->h=s->h;r->bgr555=s->bgr555;
  }else if (memory && !finished && opcode == 0x10) {
   if (arg & ~0x000f0000u) return PSP_GE_SUBSET_UNSUPPORTED;
   base = arg << 8;
  }else if (memory && !finished && opcode == 0x01) {
   address = base | arg;
   has_address = 1;
  }else if (memory && !finished && opcode == 0x12) {
   /* Through mode | 16-bit position | 5551 color. */
   if (arg != 0x00800114u) return PSP_GE_SUBSET_UNSUPPORTED;
   has_format = 1;
  }else if(opcode==0x0f){ /* FINISH must be followed by END. */
   if(finished||arg)return PSP_GE_SUBSET_MALFORMED;
   finished=1;
  }else if(opcode==0x0c){ /* Terminates list; no trailing commands. */
   if(!finished||arg||(!memory && used!=sprite_count))return PSP_GE_SUBSET_MALFORMED;
   ended=1;
  }else{
   /* Unsupported state and display-list branches fail closed. */
   return PSP_GE_SUBSET_UNSUPPORTED;
  }
 }
 if(!ended||!finished||!used)return PSP_GE_SUBSET_MALFORMED;
 staged.consumed_commands=(uint32_t)count;
 staged.consumed_sprites=(uint32_t)used;
 *out=staged;
 return PSP_GE_SUBSET_OK;
}


int psp_ge_subset_translate(const uint32_t *words, size_t count,
                            const PspGeDecodedSprite *sprites, size_t sprite_count,
                            PspGeSubsetResult *out) {
 return translate(words, count, sprites, sprite_count, NULL, 0, 0, out);
}

int psp_ge_subset_translate_memory(const uint32_t *words, size_t count,
                                   const uint8_t *memory, size_t size,
                                   uint32_t memory_base, PspGeSubsetResult *out) {
 if (!memory || !size) {
  if (out) memset(out, 0, sizeof(*out));
  return PSP_GE_SUBSET_BAD_ARGUMENT;
 }
 return translate(words, count, NULL, 0, memory, size, memory_base, out);
}

/* A guest display list is little-endian data, not a host uint32_t array.
   Decode it only after validating the complete command span. */
int psp_ge_subset_translate_guest(const uint8_t *memory, size_t size,
                                  uint32_t memory_base, uint32_t list_address,
                                  size_t word_count, PspGeSubsetResult *out) {
 if (out) memset(out, 0, sizeof *out);
 if (!out || !memory || !size || !word_count ||
     word_count > PSP_GE_SUBSET_MAX_COMMANDS ||
     (list_address & 3u) || list_address < memory_base)
  return PSP_GE_SUBSET_BAD_ARGUMENT;
 size_t offset = (size_t)(list_address - memory_base);
 if (offset > size || word_count > (size - offset) / 4u)
  return PSP_GE_SUBSET_MALFORMED;
 uint32_t words[PSP_GE_SUBSET_MAX_COMMANDS];
 for (size_t i = 0; i < word_count; ++i) {
  const uint8_t *p = memory + offset + i * 4u;
  words[i] = (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
             ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
 }
 return psp_ge_subset_translate_memory(words, word_count, memory, size,
                                       memory_base, out);
}

/* The guest bus is the source of truth for hot PSP RAM. This adapter
   deliberately rejects cold-only addresses until cross-window GE fetches
   can be made atomic and bounded. */
int psp_ge_subset_translate_bus(const PspGuestBus *bus, uint32_t list_address,
                                size_t word_count, PspGeSubsetResult *out) {
 if (out) memset(out, 0, sizeof *out);
 if (!out || !bus || !bus->hot || !bus->hot_size ||
     bus->hot_size > 32u * 1024u * 1024u)
  return PSP_GE_SUBSET_BAD_ARGUMENT;
 return psp_ge_subset_translate_guest(bus->hot, bus->hot_size,
                                      bus->hot_base, list_address,
                                      word_count, out);
}

/* Host integration boundary for CPU-produced list pointers. The emulator
   must explicitly call this after a verified graphics submission event. */
int psp_ge_subset_submit_cpu(const PspCpu *cpu, unsigned address_reg,
                             unsigned count_reg, PspGeSubsetResult *out) {
 if (out) memset(out, 0, sizeof *out);
 if (!out || !cpu || !cpu->ram || !cpu->ram_size ||
     address_reg >= 32u || count_reg >= 32u || cpu->fault)
  return PSP_GE_SUBSET_BAD_ARGUMENT;
 return psp_ge_subset_translate_guest(cpu->ram, cpu->ram_size,
                                      cpu->ram_base, cpu->gpr[address_reg],
                                      cpu->gpr[count_reg], out);
}
