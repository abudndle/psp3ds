#ifndef PSP_LOADER_H
#define PSP_LOADER_H
#include <stddef.h>
#include <stdint.h>
typedef enum { PSP_LOAD_OK = 0, PSP_LOAD_BAD_ARGUMENT = -1, PSP_LOAD_NOT_ELF = -2, PSP_LOAD_UNSUPPORTED = -3, PSP_LOAD_BAD_SEGMENT = -4, PSP_LOAD_ENTRY_INVALID = -5 } PspLoadStatus;
typedef struct { uint32_t entry; uint16_t segments; } PspLoadResult;
int psp_elf_load(const uint8_t *file, size_t len, uint8_t *ram, size_t ram_len, uint32_t ram_base, PspLoadResult *out);
#endif
