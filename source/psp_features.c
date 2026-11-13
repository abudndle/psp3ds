#include "psp_features.h"
#include "psp_benchmark.h"
#include "psp_core.h"
#include <string.h>
static const char *const names[PSP_FEATURE_COUNT]={
 "Synthetic MIPS CPU",
 "Synthetic MIPS benchmark",
 "Memory and graphics policy",
 "Checked guest data bus",
 "PSP graphics translation",
 "Commercial PSP game boot"
};
int psp_features_manifest(PspTarget target,PspFeatureManifest *out){
 if(!out)return -1;
 memset(out,0,sizeof(*out));
 if(psp_platform_budget_choose(target,&out->policy)!=0 ||
    psp_platform_budget_validate(&out->policy)!=0)return -1;
 out->target=target;
 for(unsigned i=0;i<PSP_FEATURE_COUNT;i++){
  out->features[i].id=(PspFeatureId)i;
  out->features[i].name=names[i];
  out->features[i].state=PSP_FEATURE_PLANNED;
  out->features[i].backend=PSP_BACKEND_UNIMPLEMENTED;
 }
 for(unsigned i=PSP_FEATURE_CPU_SMOKE;i<=PSP_FEATURE_MIPS_BENCHMARK;i++){
  out->features[i].state=PSP_FEATURE_AVAILABLE;
  out->features[i].backend=PSP_BACKEND_SHARED_CPU;
 }
 out->features[PSP_FEATURE_MEMORY_POLICY].state=PSP_FEATURE_AVAILABLE;
 out->features[PSP_FEATURE_MEMORY_POLICY].backend=PSP_BACKEND_INTERNAL_RAM;
 /* Guest-bus library exists and has tests; this feature is not yet wired
  * to the PSP interpreter, and only DS+Pak has an external backend. */
 out->features[PSP_FEATURE_GUEST_DATA_BUS].state=PSP_FEATURE_EXPERIMENTAL;
 out->features[PSP_FEATURE_GUEST_DATA_BUS].backend=
   target==PSP_TARGET_DS_SLOT2?PSP_BACKEND_SLOT2_DATA:PSP_BACKEND_INTERNAL_RAM;
 /* These are chosen FUTURE backends, not actual rendered PSP graphics. */
 out->features[PSP_FEATURE_GRAPHICS_TRANSLATION].backend=
   out->policy.graphics==PSP_GRAPHICS_DS_TILES?
   PSP_BACKEND_DS_2D:PSP_BACKEND_3DS_GPU;
 /* Never advertise unimplemented retail boot as a working feature. */
 out->features[PSP_FEATURE_PSP_GAME_BOOT].state=PSP_FEATURE_PLANNED;
 return 0;
}
const char *psp_feature_state_name(PspFeatureState state){
 switch(state){
  case PSP_FEATURE_AVAILABLE:return "diagnostic";
  case PSP_FEATURE_EXPERIMENTAL:return "experimental";
  case PSP_FEATURE_PLANNED:return "not implemented";
  default:return "invalid";
 }
}
int psp_feature_run(PspFeatureId id,uint32_t *detail){
 if(detail)*detail=0;
 if(!detail)return -1;
 if(id==PSP_FEATURE_CPU_SMOKE){
  uint8_t instructions[8]={1,0,8,0x24,1,0,8,0x25};
  PspCpu cpu;
  psp_cpu_init(&cpu,instructions,sizeof instructions,0x08800000u,0x08800000u);
  if(psp_cpu_step(&cpu)!=0 || psp_cpu_step(&cpu)!=0 || cpu.gpr[8]!=2u)
   return -2;
  *detail=cpu.gpr[8];
  return 0;
 }
 if(id==PSP_FEATURE_MIPS_BENCHMARK){
  PspBenchResult result;
  if(psp_benchmark_run(16,&result)!=0 || result.executed_instructions!=2048u)
   return -2;
  *detail=result.executed_instructions;
  return 0;
 }
 return -1;
}
