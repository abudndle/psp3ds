#include "psp_lazy_backends.h"
#include <stdint.h>
/* DS/DS+verified Pak/DSi: demonstrate the demand-only policy without
 * allocating backend resources. No retail PSP work is synthesized here. */
typedef struct {uint32_t start[3],work[3],stop[3];} Smoke;
static int begin(void *opaque,PspLazySubsystem sub){
 ((Smoke*)opaque)->start[sub]++;return 0;
}
static int execute(void *opaque,PspLazySubsystem sub){
 ((Smoke*)opaque)->work[sub]++;return 0;
}
static void finish(void *opaque,PspLazySubsystem sub){
 ((Smoke*)opaque)->stop[sub]++;
}
int psp_nds_lazy_backends_smoke(PspTarget target){
 PspLazyBackends s={0};
 Smoke counts={{0},{0},{0}};
 PspLazyBackendOps ops={begin,execute,finish,&counts};
 PspLazyShutdown report;
 unsigned processed=99;
 if(psp_lazy_backends_init(&s,target)!=0)return -1;
 for(unsigned i=0;i<8;i++){
  if(psp_lazy_backends_pump(&s,&ops,&processed)!=0||processed!=0u)
   return -1;
 }
 if(s.initialized_count!=0u)return -1;
 if(psp_lazy_backends_request(&s,PSP_LAZY_AUDIO,2u)!=0 ||
    psp_lazy_backends_request(&s,PSP_LAZY_GPU,2u)!=0 ||
    psp_lazy_backends_request(&s,PSP_LAZY_UMD,2u)!=0)return -1;
 unsigned total=0;
 for(unsigned attempts=0;attempts<6u && total<6u;attempts++){
  if(psp_lazy_backends_pump(&s,&ops,&processed)!=1)return -1;
  total+=processed;
 }
 if(total!=6u || psp_lazy_backends_close(&s,&ops,&report)!=0)
  return -1;
 for(unsigned i=0;i<3u;i++)
  if(counts.start[i]!=1u||counts.work[i]!=2u||counts.stop[i]!=1u)
   return -1;
 return report.completed_units==6u&&report.pending_units==0u?0:-1;
}
