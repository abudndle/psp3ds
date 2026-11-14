#ifndef PSP_3DS_STORAGE_H
#define PSP_3DS_STORAGE_H
#include <stdint.h>
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif

/* Official old/New 3DS SD/SDHC support: up to 32 GiB storage.
 * On FAT32 a single file cannot exceed 4 GiB minus one byte.
 * SD space is storage, never PSP writable RAM or executable ARM memory.
 * FSFILE_Read uses u64 file offsets; do not truncate to signed fseek(long).
 */
#define PSP_3DS_SD_OFFICIAL_MAX_BYTES UINT64_C(34359738368)
#define PSP_3DS_SD_FAT32_MAX_FILE UINT64_C(4294967295)
#define PSP_3DS_SD_MAX_CHUNK 4096u

/* Return 0 only if the entire requested chunk was read. A callback may
 * partially overwrite the supplied buffer even when returning -1: the
 * reader stages output and commits only after the callback succeeds. */
typedef int (*Psp3dsStorageRead)(void *opaque,uint64_t offset,
                                  uint8_t *data,size_t length);
typedef struct {
 Psp3dsStorageRead read;
 void *opaque;
 uint64_t length;
 uint32_t calls;
 uint32_t read_bytes;
} Psp3dsStorage;

int psp_3ds_storage_init(Psp3dsStorage *storage,uint64_t file_length,
                         Psp3dsStorageRead read,void *opaque);
int psp_3ds_storage_read(Psp3dsStorage *storage,uint64_t offset,
                         uint8_t *dst,size_t bytes);
#ifdef __cplusplus
}
#endif
#endif
