#ifndef PSP_NATIVE_TEXTURE_H
#define PSP_NATIVE_TEXTURE_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Exact GE_TFMT_* numeric values in the pinned upstream PPSSPP revision.
 * CLUT and compressed textures deliberately return UNSUPPORTED until
 * implemented; format conversion is not a replacement for PSP GPU state. */
typedef enum {
 PSP_TEXTURE_5650 = 0,
 PSP_TEXTURE_5551 = 1,
 PSP_TEXTURE_4444 = 2,
 PSP_TEXTURE_8888 = 3
} PspNativeTextureFormat;

typedef struct {
 uint16_t width, height;
 uint16_t buffer_width; /* PSP texture pitch in pixels; may exceed width */
 uint8_t format;
 uint8_t swizzled;      /* PSP 16-byte by 8-row block layout */
} PspNativeTexture;

typedef struct {
 uint16_t x, y, width, height;
} PspNativeTextureRegion;

typedef enum {
 PSP_TEXTURE_OK = 0,
 PSP_TEXTURE_BAD_ARGUMENT = -1,
 PSP_TEXTURE_UNSUPPORTED = -2,
 PSP_TEXTURE_TRUNCATED = -3
} PspNativeTextureStatus;

/* Converts only a requested subrectangle to linear RGBA8. The caller owns
 * both buffers; no heap, graphics API, C++ runtime or SIMD required.
 * This allows small DS tiles to decode without allocating a 480x272 image.
 * Rejects overlapping buffers and invalid bounds before any writes.
 * Non-multiple-of-8 swizzled heights require padded source tile rows. */
int psp_native_texture_decode_region(
 const PspNativeTexture *texture,
 const uint8_t *source, size_t source_size,
 const PspNativeTextureRegion *region,
 uint8_t *rgba, size_t rgba_stride, size_t rgba_size);

#ifdef __cplusplus
}
#endif
#endif
