#include "psp_slot2_budget.h"
#include <assert.h>
#include <stdio.h>
int main(void){
 PspPlatformBudget b;
 PspSlot2Result p={PSP_SLOT2_UNAVAILABLE,0,0};
 assert(psp_slot2_budget_select(&p,&b)==0);
 assert(b.target==PSP_TARGET_DS && b.cold_data_budget==0);
 p.status=PSP_SLOT2_UNVERIFIED;p.detected_bytes=4u*1024u*1024u;
 assert(psp_slot2_budget_select(&p,&b)==0);
 p.status=PSP_SLOT2_RW_VERIFIED;p.probed_locations=1;
 assert(psp_slot2_budget_select(&p,&b)==0);
 p.probed_locations=2;p.detected_bytes=1024u*1024u;
 assert(psp_slot2_budget_select(&p,&b)==1);
 assert(b.target==PSP_TARGET_DS_SLOT2 && b.cold_data_budget==1024u*1024u);
 p.detected_bytes=8u*1024u*1024u;
 assert(psp_slot2_budget_select(&p,&b)==1);
 assert(b.cold_data_budget==2u*1024u*1024u);
 p.detected_bytes=33u*1024u*1024u;
 assert(psp_slot2_budget_select(&p,&b)==0);
 assert(psp_slot2_budget_select(0,&b)==-1);
 assert(psp_slot2_budget_select(&p,0)==-1);
 puts("PASS: Slot-2 runtime budget selection and fallback");
 return 0;
}
