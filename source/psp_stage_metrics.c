#include "psp_stage_metrics.h"
#include <stdint.h>
#include <string.h>

int psp_stage_metrics_init(PspStageMetrics *m,PspTarget target){
 if(!m||(unsigned)target>=PSP_TARGET_COUNT)return -1;
 memset(m,0,sizeof(*m));m->target=target;return 0;
}
int psp_stage_metrics_record(PspStageMetrics *m,PspStageKind kind,
                             uint32_t us,uint32_t bytes,int error){
 if(!m||(unsigned)m->target>=PSP_TARGET_COUNT ||
    (unsigned)kind>=PSP_STAGE_COUNT)return -1;
 PspStageCounter *counter=&m->stage[kind];
 if(counter->measured_operations==UINT32_MAX ||
    us>UINT64_MAX-counter->cumulative_us ||
    bytes>UINT64_MAX-counter->transferred_bytes ||
    (error && counter->errors==UINT32_MAX))return -1;
 counter->measured_operations++;
 counter->cumulative_us+=us;
 counter->transferred_bytes+=bytes;
 if(us>counter->max_us)counter->max_us=us;
 if(error)counter->errors++;
 return 0;
}
int psp_stage_metrics_memory(PspStageMetrics *m,uint32_t used,
                             uint32_t free_bytes,uint32_t reserve){
 if(!m||(unsigned)m->target>=PSP_TARGET_COUNT ||
    !used||m->memory_samples==UINT32_MAX ||
    (uint64_t)used+free_bytes>UINT32_MAX ||
    reserve>used+free_bytes)return -1;
 if(!m->memory_sampled){
  m->smallest_application_free_bytes=free_bytes;
  m->minimum_reserved_free_bytes=reserve;
  m->memory_sampled=1;
 }else{
  if(free_bytes<m->smallest_application_free_bytes)
   m->smallest_application_free_bytes=free_bytes;
  if(reserve>m->minimum_reserved_free_bytes)
   m->minimum_reserved_free_bytes=reserve;
 }
 if(used>m->peak_application_used_bytes)
  m->peak_application_used_bytes=used;
 m->memory_samples++;
 return 0;
}
int psp_stage_metrics_average(const PspStageMetrics *m,
                              PspStageKind kind,uint32_t *out){
 if(out)*out=0;
 if(!m||!out||(unsigned)kind>=PSP_STAGE_COUNT||
    !m->stage[kind].measured_operations)return -1;
 const uint64_t avg=m->stage[kind].cumulative_us/
                     m->stage[kind].measured_operations;
 if(avg>UINT32_MAX)return -1;
 *out=(uint32_t)avg;return 0;
}
const char *psp_stage_metrics_name(PspStageKind kind){
 switch(kind){
  case PSP_STAGE_CPU:return "cpu_guest";
  case PSP_STAGE_GPU_SUBMIT:return "gpu_submission_cpu";
  case PSP_STAGE_STORAGE:return "storage_read";
  case PSP_STAGE_AUDIO:return "audio_processing";
  default:return "unknown";
 }
}
