#ifndef PSP_GE_SUBSET_H
#define PSP_GE_SUBSET_H
#include "psp_scene_batch.h"
#include <stdint.h>
#include <stddef.h>
#define PSP_GE_SUBSET_MAX_COMMANDS 128u
#define PSP_GE_SUBSET_MAX_SPRITES PSP_SCENE_MAX_RECTS
typedef struct {
 uint16_t x,y,w,h;
 uint16_t bgr555;
} PspGeDecodedSprite;
typedef struct {
 PspSceneBatch batch;
 uint32_t consumed_commands;
 uint32_t consumed_sprites;
} PspGeSubsetResult;
typedef enum {
 PSP_GE_SUBSET_OK=0,
 PSP_GE_SUBSET_BAD_ARGUMENT=-1,
 PSP_GE_SUBSET_UNSUPPORTED=-2,
 PSP_GE_SUBSET_MALFORMED=-3,
 PSP_GE_SUBSET_OVERFLOW=-4
} PspGeSubsetStatus;
int psp_ge_subset_translate(const uint32_t *words,size_t word_count,
                            const PspGeDecodedSprite *sprites,size_t sprite_count,
                            PspGeSubsetResult *out);
/* Reads opaque, flat-color sprite pairs from a bounded guest memory window. */
int psp_ge_subset_translate_memory(const uint32_t *words, size_t word_count,
                                   const uint8_t *memory, size_t size,
                                   uint32_t memory_base, PspGeSubsetResult *out);
/* Explicit PSP-native 480x272 viewport variant. The old function keeps
 * 256x192 synthetic-scene bounds for existing tests and renderers.
 * This accepts only opaque, flat-color through-mode sprite pairs. */
int psp_ge_subset_translate_psp_memory(const uint32_t *words, size_t word_count,
                                       const uint8_t *memory, size_t size,
                                       uint32_t memory_base, PspGeSubsetResult *out);
#endif
