#include "psp_imports.h"
#include <string.h>
/* PSP IoFileMgrForUser NIDs confirmed against PSP library documentation.
 * Unknown calls are not treated as successfully implemented. */
static const PspExport known_exports[] = {
 {"IoFileMgrForUser",0x109f50bcu,PSP_HLE_IO_OPEN},
 {"IoFileMgrForUser",0x6a638d83u,PSP_HLE_IO_READ},
 {"IoFileMgrForUser",0x68963324u,PSP_HLE_IO_LSEEK32},
 {"IoFileMgrForUser",0x810c4bc3u,PSP_HLE_IO_CLOSE}
};
int psp_imports_link_known(PspCpu *cpu,PspHle *hle,
                           const PspImport *imports,size_t count){
 return psp_imports_link(cpu,hle,imports,count,known_exports,
                         sizeof known_exports/sizeof known_exports[0]);
}

int psp_imports_known_service(const char *library,uint32_t nid,
                              PspHleService *service){
 if(service)*service=(PspHleService)0;
 if(!library||!service||!*library)return -1;
 for(size_t i=0;i<sizeof known_exports/sizeof known_exports[0];i++){
  const PspExport *known=&known_exports[i];
  if(known->nid==nid && strcmp(known->library,library)==0){
   *service=known->service;
   return 1;
  }
 }
 return 0;
}
