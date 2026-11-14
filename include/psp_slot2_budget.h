#ifndef PSP_SLOT2_BUDGET_H
#define PSP_SLOT2_BUDGET_H
#include "psp_platform_budget.h"
#include "psp_slot2_probe.h"
/* Select a fail-closed runtime budget after a trusted platform adapter has
 * performed a Slot-2 probe. Does not allocate or map external memory.
 * Returns 1 when verified expansion is enabled, 0 for DS fallback,
 * and -1 for invalid arguments. */
int psp_slot2_budget_select(const PspSlot2Result *probe,
                            PspPlatformBudget *out);
#endif
