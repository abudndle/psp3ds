#include "psp_stage_metrics.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

int main(void){
 for(int i=0;i<PSP_TARGET_COUNT;i++){
  PspStageMetrics m;
  assert(psp_stage_metrics_init(&m,(PspTarget)i)==0);
  for(int type=0;type<PSP_STAGE_COUNT;type++){
   uint32_t avg=12345u;
   assert(psp_stage_metrics_average(&m,(PspStageKind)type,&avg)==-1);
   assert(avg==0u);
   assert(m.stage[type].measured_operations==0u);
  }
  /* Each recorded measurement belongs to exactly one category. */
  assert(psp_stage_metrics_record(&m,PSP_STAGE_CPU,120u,4096u,0)==0);
  assert(psp_stage_metrics_record(&m,PSP_STAGE_CPU,80u,4096u,0)==0);
  assert(psp_stage_metrics_record(&m,PSP_STAGE_GPU_SUBMIT,300u,64u,0)==0);
  assert(psp_stage_metrics_record(&m,PSP_STAGE_STORAGE,500u,2048u,1)==0);
  uint32_t avg=0;
  assert(psp_stage_metrics_average(&m,PSP_STAGE_CPU,&avg)==0 &&
         avg==100u);
  assert(psp_stage_metrics_average(&m,PSP_STAGE_GPU_SUBMIT,&avg)==0 &&
         avg==300u);
  assert(psp_stage_metrics_average(&m,PSP_STAGE_STORAGE,&avg)==0 &&
         avg==500u);
  assert(m.stage[PSP_STAGE_STORAGE].errors==1u);
  assert(m.stage[PSP_STAGE_AUDIO].measured_operations==0u);
  assert(psp_stage_metrics_average(&m,PSP_STAGE_AUDIO,&avg)==-1 &&
         avg==0u);
  assert(psp_stage_metrics_memory(&m,16000u,8000u,4000u)==0);
  assert(psp_stage_metrics_memory(&m,20000u,4000u,4000u)==0);
  assert(m.peak_application_used_bytes==20000u);
  assert(m.smallest_application_free_bytes==4000u);
  assert(m.minimum_reserved_free_bytes==4000u);
  assert(m.memory_samples==2u);
  assert(psp_stage_metrics_memory(&m,UINT32_MAX,1u,4000u)==-1);
  assert(psp_stage_metrics_record(&m,(PspStageKind)9u,1u,0,0)==-1);
  assert(strcmp(psp_stage_metrics_name(PSP_STAGE_CPU),"cpu_guest")==0);
  assert(strcmp(psp_stage_metrics_name(PSP_STAGE_AUDIO),
               "audio_processing")==0);
 }
 PspStageMetrics m;
 assert(psp_stage_metrics_init(&m,PSP_TARGET_COUNT)==-1);
 assert(psp_stage_metrics_init(NULL,PSP_TARGET_DS)==-1);
 assert(psp_stage_metrics_record(NULL,PSP_STAGE_CPU,0,0,0)==-1);
 assert(psp_stage_metrics_average(NULL,PSP_STAGE_CPU,NULL)==-1);
 puts("PASS: separate measured CPU/GPU/IO/audio timing and RAM peaks for all five device profiles");
 return 0;
}
