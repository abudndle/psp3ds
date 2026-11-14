#include "psp_rom_file.h"
#include "psp_rom_pager.h"
#include <limits.h>
int psp_rom_file_read(void *opaque,uint32_t off,uint8_t *dst,size_t len){
 FILE *file=(FILE*)opaque;
 if(!file||!dst||!len||len>PSP_ROM_PAGER_PAGE_BYTES||
    off>LONG_MAX || (uint64_t)off+len>PSP_ROM_PAGER_MAX_IMAGE)
  return -1;
 /* fseek is bounded to <=512MB, thus fits a 32-bit long on libnds.
  * FILE* must be opened by the frontend and remain valid during paging. */
 if(fseek(file,(long)off,SEEK_SET)!=0)return -1;
 return fread(dst,1,len,file)==len?0:-1;
}
