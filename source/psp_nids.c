#include "psp_imports.h"
/* PSP IoFileMgrForUser NIDs confirmed against PSP library documentation.
 * Unknown calls are not treated as successfully implemented. */
static const PspExport known_exports[] = {
 {"Kernel_Library",0x092968f4u,PSP_HLE_INTR_SUSPEND},
 {"Kernel_Library",0x5f10d406u,PSP_HLE_INTR_RESUME},
 {"ModuleMgrForUser",0xd8b73127u,PSP_HLE_MODULE_ID_BY_ADDRESS},
 {"ThreadManForUser",0x446d8de6u,PSP_HLE_KERNEL_CREATE_THREAD},
 {"ThreadManForUser",0xf475845du,PSP_HLE_KERNEL_START_THREAD},
 {"SysMemUserForUser",0x358ca1bbu,PSP_HLE_SET_COMPILED_SDK},
 {"SysMemUserForUser",0xfc114573u,PSP_HLE_GET_COMPILED_SDK},
 {"SysMemUserForUser",0xf77d77cbu,PSP_HLE_SET_COMPILER_VERSION},
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
