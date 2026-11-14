#include "psp_slot2_budget.h"
int psp_slot2_budget_select(const PspSlot2Result *probe,
                            PspPlatformBudget *out) {
 if(!probe || !out)return -1;
 PspPlatformBudget budget;
 if(psp_platform_budget_choose(PSP_TARGET_DS,&budget))return -1;
 if(probe->status==PSP_SLOT2_RW_VERIFIED &&
    probe->detected_bytes>=4096u &&
    probe->detected_bytes<=32u*1024u*1024u &&
    !(probe->detected_bytes&1u) &&
    probe->probed_locations>=2u) {
  PspPlatformBudget expanded;
  if(psp_platform_budget_choose(PSP_TARGET_DS_SLOT2,&expanded))return -1;
  /* A sampled probe does not prove all bytes are safe. This is a policy
   * ceiling, not permission to access or allocate Slot-2 addresses. */
  if(probe->detected_bytes<expanded.cold_data_budget)
   expanded.cold_data_budget=probe->detected_bytes;
  budget=expanded;
 }
 if(psp_platform_budget_validate(&budget))return -1;
 *out=budget;
 return budget.has_slot2?1:0;
}
