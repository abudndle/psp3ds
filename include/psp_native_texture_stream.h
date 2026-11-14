#ifndef PSP_NATIVE_TEXTURE_STREAM_H
#define PSP_NATIVE_TEXTURE_STREAM_H
#include "psp_native_texture.h"
#include <stdint.h>
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif

/* Decode exactly a requested PSP GE direct-color rectangle from an
 * immutable stream, without loading the complete PSP texture into RAM.
 * Suitable for a user-owned SD file, NitroFS stream or cache.
 *
 * Scratch <= 2,560 bytes on the stack (linear row 2,048 bytes +
 * four 128-byte PSP swizzle blocks). The caller owns the decoded
 * rectangle, which can be as small as a DS 8x8 tile (256 RGBA bytes).
 * Callback work is on demand. No GPU allocation or firmware emulation.
 */
#define PSP_TEXTURE_STREAM_LINEAR_BYTES 2048u
#define PSP_TEXTURE_STREAM_SWIZZLE_BYTES 128u
#define PSP_TEXTURE_STREAM_SWIZZLE_SLOTS 4u
#define PSP_TEXTURE_STREAM_IO_ERROR (-4)

typedef int (*PspTextureStreamRead)(void *opaque,uint64_t file_offset,
                                    uint8_t *dst,size_t bytes);
typedef struct {
 uint32_t source_reads;
 uint32_t source_bytes;
 uint32_t cached_blocks;
 uint32_t decoded_pixels;
} PspTextureStreamStats;

/* source_bytes counts the texture's total available storage (including
 * stride and PSP-swizzle padding); source_base is a 64-bit offset within
 * a file. The callback must read exactly the requested bytes or fail.
 * On callback failure dst may contain a partially decoded rectangle:
 * do not present or cache the output until this function returns OK. */
int psp_native_texture_decode_stream(
 const PspNativeTexture *texture,
 uint64_t source_base,uint64_t source_bytes,
 const PspNativeTextureRegion *region,
 PspTextureStreamRead read,void *opaque,
 uint8_t *rgba,size_t rgba_stride,size_t rgba_bytes,
 PspTextureStreamStats *stats);

#ifdef __cplusplus
}
#endif
#endif
