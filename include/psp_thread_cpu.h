#ifndef PSP_THREAD_CPU_H
#define PSP_THREAD_CPU_H
#include "psp_threads.h"
#include "psp_hle.h"

/* Host-side cooperative MIPS context switching: not PSP firmware threads.
 * All guest threads share the same mapped RAM and HLE dispatcher.
 * The caller owns PspThreads, the live CPU and the underlying guest RAM. */
typedef struct PspThreadCpu {
 PspThreads *scheduler;
 PspCpu *live;
 PspHle *hle;
 uint8_t *ram;
 size_t ram_size;
 uint32_t ram_base;
 uint32_t active_id;
 PspCpu saved[PSP_THREADS_MAX];
 uint32_t slot_id[PSP_THREADS_MAX];
} PspThreadCpu;

int psp_thread_cpu_init(PspThreadCpu *runtime, PspThreads *scheduler,
                        PspCpu *live, PspHle *hle);
/* Create a runnable task with its own initialized integer CPU state. */
int32_t psp_thread_cpu_spawn(PspThreadCpu *runtime, uint8_t priority,
                             uint32_t entry_pc, uint32_t stack_pointer);
/* Switch only to a runnable task; -1 means invalid/not runnable.
 * Live state is saved without changing its guest-memory mapping. */
int psp_thread_cpu_switch(PspThreadCpu *runtime, uint32_t id);
/* Pick one runnable task, switch, then run exactly one MIPS/HLE step.
 * Return 2 when no task is runnable; otherwise return the HLE-step status. */
int psp_thread_cpu_step(PspThreadCpu *runtime);
/* Copy the current CPU into its saved slot (including delay-slot state). */
int psp_thread_cpu_flush(PspThreadCpu *runtime);
#endif
