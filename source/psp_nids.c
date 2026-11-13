#include "psp_imports.h"
/* PSP IoFileMgrForUser NIDs confirmed against PSP library documentation.
 * Unknown calls are not treated as successfully implemented. */
static const PspExport known_exports[] = {
 {"sceCtrl",0x1f4011e6u,PSP_HLE_CTRL_SET_MODE},
 {"sceCtrl",0x6a2774f3u,PSP_HLE_CTRL_SET_CYCLE},
 {"IoFileMgrForUser",0x27eb27b8u,PSP_HLE_IO_LSEEK64},
 {"sceRtc",0x4cfa57b0u,PSP_HLE_RTC_CLOCK},
 {"sceUmdUser",0x8ef08fceu,PSP_HLE_UMD_WAIT},
 {"sceUmdUser",0xc6183d47u,PSP_HLE_UMD_ACTIVATE},
 {"sceUmdUser",0x46ebb729u,PSP_HLE_UMD_CHECK},
 {"sceUmdUser",0xaee7404du,PSP_HLE_UMD_REGISTER},
 {"scePower",0x04b7766eu,PSP_HLE_POWER_REGISTER},
 {"ThreadManForUser",0xe81caf8fu,PSP_HLE_CREATE_CALLBACK},
 {"LoadExecForUser",0x4ac57943u,PSP_HLE_REGISTER_EXIT_CALLBACK},
 {"sceImpose",0x36aa6e91u,PSP_HLE_IMPOSE_LANGUAGE},
 {"UtilsForUser",0x6ad345d7u,PSP_HLE_SET_GPO},
 {"IoFileMgrForUser",0x42ec03acu,PSP_HLE_IO_WRITE},
 {"StdioForUser",0x172d316eu,PSP_HLE_STDIN},
 {"StdioForUser",0xa6bab2e9u,PSP_HLE_STDOUT},
 {"StdioForUser",0xf78ba90au,PSP_HLE_STDERR},
 {"SysMemUserForUser",0x237dbd4fu,PSP_HLE_PART_ALLOC},
 {"SysMemUserForUser",0x9d9a5ba1u,PSP_HLE_PART_HEAD},
 {"SysMemUserForUser",0xb6d61d02u,PSP_HLE_PART_FREE},
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
