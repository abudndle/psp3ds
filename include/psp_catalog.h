#ifndef PSP_CATALOG_H
#define PSP_CATALOG_H
#include "psp_core.h"
#include <stddef.h>
#define PSP_CATALOG_FILENAME_MAX 128
/* Nonrecursive directory inventory; does not open or execute game files. */
typedef struct {
 char filename[PSP_CATALOG_FILENAME_MAX];
 PspFileType type;
} PspCatalogItem;
/* 0 success, -1 bad arguments/I/O, -2 catalog capacity exceeded. */
int psp_catalog_scan(const char *rom_directory,PspCatalogItem *items,
                     size_t capacity,size_t *count);
#endif
