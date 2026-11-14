#ifndef PSP_MEMORY_GUARD_H
#define PSP_MEMORY_GUARD_H
#include "psp_ds_memory.h"
#include "psp_3ds_memory.h"
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

/* ALL game-dependent heap allocations must use this guard in future game
 * runtimes. It cannot account for direct malloc, third-party libraries,
 * graphics VRAM/linear allocations, thread stacks or leaked OS handles.
 * No retail PSP game execution is currently connected to this API. */
#define PSP_MEMORY_GUARD_SLOTS 32u
#define PSP_MEMORY_GUARD_CLASSES 4u
typedef enum {
 PSP_MEMORY_GUEST=0, PSP_MEMORY_TEXTURE=1,
 PSP_MEMORY_CACHE=2, PSP_MEMORY_IO=3
} PspMemoryClass;
typedef void *(*PspGuardMalloc)(void *opaque,size_t size);
typedef void (*PspGuardFree)(void *opaque,void *pointer);
typedef struct {
 uint32_t total_limit;
 uint32_t class_limit[PSP_MEMORY_GUARD_CLASSES];
 /* Observed, not total physically installed RAM: temporary touched heap
  * size on DS; live free APPLICATION bytes from OS on 3DS. */
 uint32_t observed_available_bytes;
 uint32_t reserved_outside_guard;
 PspTarget target;
} PspMemoryGuardPolicy;
typedef struct {
 void *pointer;
 uint32_t bytes;
 PspMemoryClass category;
} PspGuardAllocation;
typedef struct {
 PspMemoryGuardPolicy policy;
 uint32_t soft_total_limit;
 uint32_t soft_class_limit[PSP_MEMORY_GUARD_CLASSES];
 PspGuardMalloc allocate;
 PspGuardFree deallocate;
 void *opaque;
 PspGuardAllocation tracked[PSP_MEMORY_GUARD_SLOTS];
 uint32_t allocated_bytes;
 uint32_t peak_bytes;
 uint32_t per_class[PSP_MEMORY_GUARD_CLASSES];
 uint32_t peak_class[PSP_MEMORY_GUARD_CLASSES];
 uint32_t denied_requests;
 uint32_t allocator_failures;
 uint32_t successful_allocations;
 uint32_t successful_releases;
 uint32_t outstanding_allocations;
 uint32_t initialized;
} PspMemoryGuard;
typedef struct {
 uint32_t peak_bytes;
 uint32_t end_outstanding_bytes;
 uint32_t reclaimed_bytes;
 uint32_t reclaimed_allocations;
 uint32_t denied_requests;
 uint32_t allocator_failures;
 uint32_t successful_allocations;
 uint32_t successful_releases;
} PspMemoryGuardReport;

/* DS: only 75% of a previously touched/released temporary heap probe
 * is eligible, with no external Slot-2 bytes counted as heap. */
int psp_memory_guard_policy_ds(const PspDsMemoryPlan *plan,
                               PspMemoryGuardPolicy *out);
/* 3DS: never spend the application's reserve. The total budget derives
 * from measured free bytes, not total physical FCRAM. */
int psp_memory_guard_policy_3ds(const Psp3dsMemoryPlan *plan,
                                PspMemoryGuardPolicy *out);

/* Guard objects MUST begin zero-initialized and MUST be closed before
 * reinitialization. The allocator/deallocator pair is optional; both
 * NULL uses standard malloc/free. They must not return duplicate pointers.
 * Class budgets and aggregate budgets must both pass before allocation. */
int psp_memory_guard_init(PspMemoryGuard *guard,
                          const PspMemoryGuardPolicy *policy,
                          PspGuardMalloc allocate,PspGuardFree release,
                          void *opaque);
int psp_memory_guard_acquire(PspMemoryGuard *guard,PspMemoryClass category,
                             size_t bytes,void **result);
int psp_memory_guard_release(PspMemoryGuard *guard,void *pointer);
/* Tighten FUTURE admissions as the game experiences pressure, without
 * invalidating existing guest pointers. Soft caps may be lower than
 * current occupancy; new allocations are denied until releases occur.
 * All values must stay within the unchanged hard policy caps. */
int psp_memory_guard_set_soft_limits(PspMemoryGuard *guard,
                                     uint32_t total_limit,
                                     const uint32_t class_limits[
                                       PSP_MEMORY_GUARD_CLASSES]);
/* Records how much was still allocated and reclaims ALL owned pointers.
 * A nonzero reclaimed count indicates a missing explicit release path
 * in the runtime even though final session teardown repairs the leak.
 * Does not silently claim that untracked allocations are leak-free. */
int psp_memory_guard_close(PspMemoryGuard *guard,PspMemoryGuardReport *report);
#ifdef __cplusplus
}
#endif
#endif
