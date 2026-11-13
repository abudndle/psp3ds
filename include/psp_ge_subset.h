#ifndef PSP_GE_SUBSET_H
#define PSP_GE_SUBSET_H
#include "psp_scene_batch.h"
#include <stdint.h>
#include <stddef.h>
/* Highly restricted PSP GE command decoder for an ORIGINAL synthetic 2D
 * sprite fixture. Actual PSP VTYPE/VADDR vertex decoding, render state,
 * DMA/display-list control flow and textured sprites are NOT implemented.
 *
 * Each accepted PRIM = (0x04 << 24) | (6 << 16) | 2: PSP sprite primitive
 * type 6 with exactly two vertices. Caller explicitly supplies a validated
 * predecoded rectangle corresponding to that primitive. No other state
 * changing opcode is accepted. This is not a retail-game GE translator. */
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
#endif
