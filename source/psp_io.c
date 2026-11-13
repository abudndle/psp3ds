#include "psp_io.h"
#include <string.h>
#include <stdint.h>
#include <limits.h>

void psp_io_init(PspIo *io, PspUmd *umd) {
    if (!io) return;
    memset(io, 0, sizeof(*io));
    io->umd = umd;
}
static PspIoFile *get_file(PspIo *io, int32_t fd) {
    if (!io || !io->umd || io->umd->kind == PSP_UMD_NONE ||
        fd < 3 || fd >= (int32_t)(3 + PSP_IO_MAX_FILES)) return NULL;
    PspIoFile *file = &io->files[fd - 3];
    if (!file->used || file->media_generation != io->umd->generation)
        return NULL;
    return file;
}
int32_t psp_io_open(PspIo *io, const char *path, uint32_t flags) {
    if (!io || !io->umd || io->umd->kind == PSP_UMD_NONE || !path ||
        flags != PSP_IO_RDONLY || strncmp(path, "disc0:/", 7) != 0)
        return -1;
    /* No host paths, traversal, or writable access. PSP paths stay inside disc. */
    const char *relative = path + 7;
    size_t len = strlen(relative);
    if (!len || len > 240 || strstr(relative, "..") || strchr(relative, '\\'))
        return -1;
    char absolute[256];
    absolute[0] = '/';
    memcpy(absolute + 1, relative, len + 1);
    PspDiscEntry entry;
    if (psp_umd_find(io->umd, absolute, &entry) != 0 || entry.is_dir)
        return -1;
    for (unsigned i = 0; i < PSP_IO_MAX_FILES; ++i) {
        PspIoFile *f = &io->files[i];
        /* Reclaim stale descriptors after a disc swap. */
        if (!f->used || f->media_generation != io->umd->generation) {
            *f = (PspIoFile){0};
            f->entry = entry;
            f->media_generation = io->umd->generation;
            f->used = 1;
            return (int32_t)(i + 3);
        }
    }
    return -1;
}
int32_t psp_io_read(PspIo *io, int32_t fd, void *dst, uint32_t size) {
    PspIoFile *f = get_file(io, fd);
    if (!f || (!dst && size) || size > INT32_MAX) return -1;
    if (f->offset > f->entry.size) return 0;
    uint32_t available = f->entry.size - f->offset;
    uint32_t count = size < available ? size : available;
    if (!count) return 0;
    if (psp_umd_read(io->umd, f->entry, f->offset, dst, count) != 0) return -1;
    f->offset += count;
    return (int32_t)count;
}
int32_t psp_io_seek32(PspIo *io, int32_t fd, int32_t offset, uint32_t whence) {
    PspIoFile *f = get_file(io, fd);
    if (!f || whence > 2) return -1;
    int64_t base = whence == 0 ? 0 :
                   whence == 1 ? (int64_t)f->offset : (int64_t)f->entry.size;
    int64_t position = base + (int64_t)offset;
    /* 32-bit PSP lseek32 returns a signed offset; reject overflow. */
    if (position < 0 || position > INT32_MAX) return -1;
    f->offset = (uint32_t)position;
    return (int32_t)position;
}
int32_t psp_io_close(PspIo *io, int32_t fd) {
    PspIoFile *f = get_file(io, fd);
    if (!f) return -1;
    memset(f, 0, sizeof(*f));
    return 0;
}
