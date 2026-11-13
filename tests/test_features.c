#include "psp_features.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
int main(void){
 PspFeatureManifest model[PSP_TARGET_COUNT];
 for(unsigned t=0;t<PSP_TARGET_COUNT;t++){
  assert(psp_features_manifest((PspTarget)t,&model[t])==0);
  assert(model[t].target==(PspTarget)t);
  for(unsigned i=0;i<PSP_FEATURE_COUNT;i++){
   const PspFeatureDescriptor *feature=&model[t].features[i];
   assert(feature->id==(PspFeatureId)i);
   assert(feature->name && feature->name[0]!=0);
   assert(feature->state<=PSP_FEATURE_PLANNED);
   assert(feature->backend<=PSP_BACKEND_3DS_FRAMEBUFFER);
   assert(!strcmp(feature->name,model[0].features[i].name));
   assert(feature->state==model[0].features[i].state);
  }
  assert(model[t].features[PSP_FEATURE_CPU_SMOKE].state==PSP_FEATURE_AVAILABLE);
  assert(model[t].features[PSP_FEATURE_MIPS_BENCHMARK].state==PSP_FEATURE_AVAILABLE);
  assert(model[t].features[PSP_FEATURE_GRAPHICS_TRANSLATION].state==PSP_FEATURE_PLANNED);
  assert(model[t].features[PSP_FEATURE_PSP_GAME_BOOT].state==PSP_FEATURE_PLANNED);
  assert(model[t].features[PSP_FEATURE_SYNTHETIC_SCENE].state==PSP_FEATURE_AVAILABLE);
  uint32_t value=123;
  assert(psp_feature_run(PSP_FEATURE_CPU_SMOKE,&value)==0 && value==2);
  assert(psp_feature_run(PSP_FEATURE_MIPS_BENCHMARK,&value)==0 && value==2048);
  value=123;
  assert(psp_feature_run(PSP_FEATURE_PSP_GAME_BOOT,&value)==-1 && value==0);
 }
 assert(model[PSP_TARGET_DS].features[PSP_FEATURE_GRAPHICS_TRANSLATION].backend==
        PSP_BACKEND_DS_2D);
 assert(model[PSP_TARGET_DSI].features[PSP_FEATURE_GRAPHICS_TRANSLATION].backend==
        PSP_BACKEND_DS_2D);
 assert(model[PSP_TARGET_DS].features[PSP_FEATURE_SYNTHETIC_SCENE].backend==PSP_BACKEND_DS_2D);
 assert(model[PSP_TARGET_NEW3DS].features[PSP_FEATURE_SYNTHETIC_SCENE].backend==PSP_BACKEND_3DS_GPU);
 assert(model[PSP_TARGET_OLD3DS].features[PSP_FEATURE_GRAPHICS_TRANSLATION].backend==
        PSP_BACKEND_3DS_GPU);
 assert(model[PSP_TARGET_NEW3DS].features[PSP_FEATURE_GRAPHICS_TRANSLATION].backend==
        PSP_BACKEND_3DS_GPU);
 assert(model[PSP_TARGET_DS_SLOT2].features[PSP_FEATURE_GUEST_DATA_BUS].backend==
        PSP_BACKEND_SLOT2_DATA);
 assert(model[PSP_TARGET_DS].features[PSP_FEATURE_GUEST_DATA_BUS].backend==
        PSP_BACKEND_INTERNAL_RAM);
 assert(model[PSP_TARGET_DSI].features[PSP_FEATURE_GUEST_DATA_BUS].backend==
        PSP_BACKEND_INTERNAL_RAM);
 PspFeatureManifest invalid;
 assert(psp_features_manifest(PSP_TARGET_COUNT,&invalid)==-1);
 assert(psp_features_manifest(PSP_TARGET_DS,NULL)==-1);
 assert(psp_feature_run(PSP_FEATURE_CPU_SMOKE,NULL)==-1);
 assert(strcmp(psp_feature_state_name(PSP_FEATURE_AVAILABLE),
               psp_feature_state_name(PSP_FEATURE_PLANNED))!=0);
 puts("PASS: equal feature IDs/states on all five targets, optimized backend selection, CPU parity");
 return 0;
}
