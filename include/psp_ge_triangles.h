#ifndef PSP_GE_TRIANGLES_H
#define PSP_GE_TRIANGLES_H
#include <stdint.h>
#include <stddef.h>
#define PSP_GE_TRIANGLE_MAX_COMMANDS 128u
#define PSP_GE_TRIANGLE_MAX_TRIANGLES 32u
#define PSP_GE_TRIANGLE_WIDTH 480u
#define PSP_GE_TRIANGLE_HEIGHT 272u
typedef struct {
 uint16_t x,y,z,color555;
} PspGeTriangleVertex;
typedef struct { PspGeTriangleVertex v[3]; } PspGeTriangle;
typedef struct {
 PspGeTriangle triangles[PSP_GE_TRIANGLE_MAX_TRIANGLES];
 uint32_t count,consumed_commands;
} PspGeTriangleResult;
typedef enum {
 PSP_GE_TRIANGLE_OK=0,
 PSP_GE_TRIANGLE_BAD_ARGUMENT=-1,
 PSP_GE_TRIANGLE_UNSUPPORTED=-2,
 PSP_GE_TRIANGLE_MALFORMED=-3,
 PSP_GE_TRIANGLE_OVERFLOW=-4
} PspGeTriangleStatus;
/* Strict, bounded GE subset: opaque color5551 + signed 16-bit XYZ
 * THROUGH vertices, unindexed triangle PRIMs, FINISH, END.
 * No transform, texture, depth/alpha/blend state, clipping, or display
 * list jumps. Coordinates must fit the 480x272 PSP visible viewport.
 * The renderer can project decoded triangles into each native screen.
 * Output stays empty on every error. */
int psp_ge_triangles_decode(const uint32_t *commands,size_t count,
                            const uint8_t *guest,size_t guest_size,
                            uint32_t guest_base,PspGeTriangleResult *out);
#endif
