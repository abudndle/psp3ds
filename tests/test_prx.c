#include "psp_prx.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#define BASE 0x08800000u
static void wr16(uint8_t *p,uint16_t n){p[0]=(uint8_t)n;p[1]=(uint8_t)(n>>8);}
static void wr32(uint8_t *p,uint32_t n){for(unsigned i=0;i<4;i++)p[i]=(uint8_t)(n>>(i*8));}
int main(void){
 uint8_t ram[512]={0}; PspCpu cpu;PspPrxImports result;PspHle hle;
 psp_cpu_init(&cpu,ram,sizeof ram,BASE,BASE);psp_hle_init(&hle);
 memcpy(ram+128,"UtilsForUser",13);
 wr32(ram,BASE+128);ram[8]=5;wr16(ram+10,2);
 wr32(ram+12,BASE+160);wr32(ram+16,BASE+192);
 wr32(ram+160,0x11223344u);wr32(ram+164,0x55667788u);
 assert(psp_prx_parse_imports(&cpu,BASE,BASE+20,&result)==0);
 assert(result.count==2 && strcmp(result.imports[0].library,"UtilsForUser")==0);
 assert(result.imports[0].nid==0x11223344u);
 assert(result.imports[1].stub_address==BASE+200);
 PspExport exports[]={{"UtilsForUser",0x11223344u,PSP_HLE_SYSTEM_TIME_LOW},
                       {"UtilsForUser",0x55667788u,PSP_HLE_SYSTEM_TIME_WIDE}};
 assert(psp_imports_link(&cpu,&hle,result.imports,result.count,exports,2)==0);
 assert(hle.count==2 && ram[192]==0x08 && ram[195]==0x03);
 ram[8]=6;wr32(ram+20,BASE+240);
 assert(psp_prx_parse_imports(&cpu,BASE,BASE+24,&result)==0 && result.count==2);
 ram[9]=1;
 assert(psp_prx_parse_imports(&cpu,BASE,BASE+24,&result)==-2 && result.count==0);
 ram[9]=0;ram[8]=4;
 assert(psp_prx_parse_imports(&cpu,BASE,BASE+20,&result)==-2);
 ram[8]=5;wr32(ram+12,BASE+511);
 assert(psp_prx_parse_imports(&cpu,BASE,BASE+20,&result)==-2);
 wr32(ram+12,BASE+160);wr32(ram,BASE+511);
 assert(psp_prx_parse_imports(&cpu,BASE,BASE+20,&result)==-2);
 wr32(ram,BASE+128);
 assert(psp_prx_parse_imports(&cpu,BASE+1,BASE+20,&result)==-1);
 assert(psp_prx_parse_imports(&cpu,BASE+20,BASE,&result)==-1);
 puts("PASS: 20/24-byte PSP import tables, NIDs, bounds and linker integration");return 0;
}
