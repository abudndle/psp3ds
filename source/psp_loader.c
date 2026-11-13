#include "psp_loader.h"
#include "psp_core.h"
#include <string.h>
static uint16_t rd16(const uint8_t *p) { return (uint16_t)(p[0] | (uint16_t)p[1] << 8); }
static uint32_t rd32(const uint8_t *p) { return (uint32_t)p[0] | (uint32_t)p[1]<<8 | (uint32_t)p[2]<<16 | (uint32_t)p[3]<<24; }
int psp_elf_load(const uint8_t *file, size_t len, uint8_t *ram, size_t ram_len,
                 uint32_t ram_base, PspLoadResult *out) {
    PspFileInfo info;
    if (!file || !ram || !out || len < 52) return PSP_LOAD_BAD_ARGUMENT;
    memset(out, 0, sizeof(*out));
    if (psp_inspect(file, len, &info) != 0 || info.type != PSP_FILE_ELF)
        return PSP_LOAD_NOT_ELF;
    if (file[6] != 1 || rd16(file + 16) != 2 || rd16(file + 40) < 52)
        return PSP_LOAD_UNSUPPORTED;
    uint32_t phoff = rd32(file + 28);
    uint16_t phentsize = rd16(file + 42), phnum = rd16(file + 44);
    if (phentsize < 32 || phnum == 0 || phnum > 128) return PSP_LOAD_BAD_SEGMENT;
    if ((uint64_t)phoff + (uint64_t)phentsize * phnum > len) return PSP_LOAD_BAD_SEGMENT;
    uint32_t entry = rd32(file + 24);
    uint64_t end_ram = (uint64_t)ram_base + ram_len;
    if (entry < ram_base || (uint64_t)entry >= end_ram) return PSP_LOAD_ENTRY_INVALID;
    int entry_executable = 0;
    uint16_t loaded = 0;
    for (unsigned i = 0; i < phnum; ++i) {
        const uint8_t *ph = file + phoff + (size_t)i * phentsize;
        if (rd32(ph) != 1) continue;
        uint32_t off = rd32(ph + 4), addr = rd32(ph + 8);
        uint32_t fsz = rd32(ph + 16), msz = rd32(ph + 20);
        uint32_t flags = rd32(ph + 24);
        if (msz < fsz || (uint64_t)off + fsz > len ||
            addr < ram_base || (uint64_t)addr + msz > end_ram)
            return PSP_LOAD_BAD_SEGMENT;
        if ((flags & 1u) && (uint64_t)entry >= addr && (uint64_t)entry < (uint64_t)addr + msz)
            entry_executable = 1;
        loaded++;
    }
    if (loaded == 0 || !entry_executable) return PSP_LOAD_ENTRY_INVALID;
    for (unsigned i = 0; i < phnum; ++i) {
        const uint8_t *ph = file + phoff + (size_t)i * phentsize;
        if (rd32(ph) != 1) continue;
        uint32_t off = rd32(ph + 4), addr = rd32(ph + 8);
        uint32_t fsz = rd32(ph + 16), msz = rd32(ph + 20);
        size_t dst = (size_t)(addr - ram_base);
        memset(ram + dst, 0, msz);
        memcpy(ram + dst, file + off, fsz);
    }
    out->entry = entry; out->segments = loaded; return PSP_LOAD_OK;
}
