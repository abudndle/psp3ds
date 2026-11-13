#ifndef PSP_IO_H
#define PSP_IO_H
#include "psp_umd.h"
#include <stdint.h>
#include <stddef.h>

/* Read-only subset of PSP sceIo* operations for disc0: paths.
 * The guest-facing syscall ID is supplied by the import linker, not fixed. */
#define PSP_IO_MAX_FILES 8u
#define PSP_IO_RDONLY 0x0001u

typedef struct {
    PspDiscEntry entry;
    uint32_t offset;
    uint32_t media_generation;
    uint8_t used;
} PspIoFile;

typedef struct PspIo {
    PspUmd *umd; /* caller owns the mounted image and keeps it alive */
    PspIoFile files[PSP_IO_MAX_FILES];
} PspIo;

void psp_io_init(PspIo *io, PspUmd *umd);
/* Returns fd >= 3 or -1. Read-only disc0:/... paths only. */
int32_t psp_io_open(PspIo *io, const char *guest_path, uint32_t flags);
/* Returns bytes read (including 0 at EOF), -1 on invalid arguments or I/O failure. */
int32_t psp_io_read(PspIo *io, int32_t fd, void *dst, uint32_t size);
/* SEEK_SET/CUR/END (0/1/2); accepts offsets through EOF. */
int32_t psp_io_seek32(PspIo *io, int32_t fd, int32_t offset, uint32_t whence);
int64_t psp_io_seek64(PspIo *io, int32_t fd, int64_t offset, uint32_t whence);
/* Returns 0 or -1. */
int32_t psp_io_close(PspIo *io, int32_t fd);
#endif
