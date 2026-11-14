#ifndef PSP_STARTUP_TRACE_H
#define PSP_STARTUP_TRACE_H
#include "psp_core.h"
#include "psp_prx_inventory.h"
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* The trace is a bounded, deterministic *diagnostic*, not a PSP firmware
 * replacement. It executes only a supplied ELF's file-backed executable
 * PT_LOAD ranges, and NEVER fabricates successful syscall results.
 * Only original synthetic fixtures may enter the public regression suite.
 */
#define PSP_STARTUP_TRACE_MAX_RANGES 16u
#define PSP_STARTUP_TRACE_MAX_STEPS 8192u
#define PSP_STARTUP_MIN_STACK 4096u
#define PSP_STARTUP_MAX_STACK (2u*1024u*1024u)

typedef enum {
 PSP_STARTUP_BUDGET = 0,
 PSP_STARTUP_UNLINKED_IMPORT = 1,
 PSP_STARTUP_SYSCALL = 2,
 PSP_STARTUP_UNSUPPORTED_MIPS = 3,
 PSP_STARTUP_FPU_REQUIRED = 4,
 PSP_STARTUP_VFPU_REQUIRED = 5,
 PSP_STARTUP_MEMORY_FAULT = 6,
 PSP_STARTUP_OUTSIDE_CODE = 7,
 PSP_STARTUP_EXCEPTION = 8
} PspStartupStop;

typedef struct {
 uint32_t first, one_past_last;
} PspStartupExecutableRange;

typedef struct {
 PspStartupExecutableRange executable[PSP_STARTUP_TRACE_MAX_RANGES];
 uint8_t executable_count;
 uint32_t instruction_cap;
 const PspPrxImportInventory *imports; /* validated read-only inventory */
 /* Optional, narrowly linked HLE. Caller must link only verified NIDs
  * BEFORE tracing. Every unlinked/unknown import still stops cleanly. */
 PspHle *hle;
} PspStartupTraceConfig;

typedef struct {
 PspStartupStop stop;
 uint32_t executed;
 uint32_t start_pc;
 uint32_t stop_pc;
 uint32_t next_pc;
 uint32_t pc_fingerprint; /* FNV-1a PC path only; no executable bytes */
 uint32_t import_nid;
 uint32_t syscall_id;
 uint8_t major_opcode;
 uint8_t function_code;
 uint8_t import_known_hle;
 char import_library[PSP_PRX_MAX_NAME];
 int interpreter_status;
} PspStartupTraceReport;

/* Initialize a diagnostic PSP entry thread only when a separately
 * allocated guest arena has >= stack_bytes reserved ABOVE mapped image.
 * gp must come from already mapped module metadata; 0 is valid.
 * Assumes argv=0, argc=0 for diagnosis, NOT an authentic PSP loader ABI.
 * Never changes the code or fabricates firmware-return values. */
int psp_startup_prepare_cpu(PspCpu *cpu,uint32_t mapped_image_bytes,
                            uint32_t stack_bytes,uint32_t gp);

/* Build an executable-range whitelist from already validated ELF32/MIPS
 * ET_SCE_PRX program headers. No bytecode is executed or relocated here.
 * ELF bytes must remain external to guest RAM (as required by the mapper).
 * A zero import inventory is allowed; the caller may set it afterwards. */
int psp_startup_trace_config_elf(const uint8_t *elf,size_t elf_size,
                                 uint32_t guest_base,uint32_t guest_capacity,
                                 uint32_t instruction_cap,
                                 PspStartupTraceConfig *out);

/* Status 0 indicates a valid reproducible STOP REPORT, not a successful
 * game boot. Stop before entering any unresolved import stub; detect
 * unsupported FPU/VFPU and memory faults without guessing HLE behavior.
 * The CPU is left at the exact reproducible halt point. When a valid
 * linked HLE is supplied, its verified syscall executes with the normal
 * branch-delay behavior, while all other import stubs still stop. */
int psp_startup_trace_run(PspCpu *cpu,
                          const PspStartupTraceConfig *config,
                          PspStartupTraceReport *out);
const char *psp_startup_stop_name(PspStartupStop stop);

#ifdef __cplusplus
}
#endif
#endif
