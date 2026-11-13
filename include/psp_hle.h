#ifndef PSP_HLE_H
#define PSP_HLE_H
#include "psp_core.h"
#include <stddef.h>
#include <stdint.h>
struct PspIo;
struct PspThreadCpu;
/* PSP syscall numbers are assigned by module import resolution; no global fixed IDs. */
#define PSP_HLE_MAX_BINDINGS 64u
/* Reserved syscall range for diagnostic unresolved imports, indexed by ordinal. */
#define PSP_HLE_UNRESOLVED_BASE 0xffe00u
#define PSP_HLE_ERR_INVALID_ARGUMENT UINT32_C(0xffffffff)
typedef enum {
 PSP_HLE_SYSTEM_TIME_LOW=1,
 PSP_HLE_SYSTEM_TIME_WIDE=2,
 PSP_HLE_SYSTEM_TIME=3,
 PSP_HLE_IO_OPEN=4,
 PSP_HLE_IO_READ=5,
 PSP_HLE_IO_LSEEK32=6,
 PSP_HLE_IO_CLOSE=7,
 /* Synthetic test-only IDs: not verified PSP kernel NIDs. */
 PSP_HLE_THREAD_CREATE=8,
 PSP_HLE_THREAD_WAIT=9,
 PSP_HLE_THREAD_SIGNAL=10,
 PSP_HLE_THREAD_EXIT=11,
 PSP_HLE_SET_COMPILED_SDK=12,
 PSP_HLE_GET_COMPILED_SDK=13,
 PSP_HLE_SET_COMPILER_VERSION=14,
 PSP_HLE_KERNEL_CREATE_THREAD=15,
 PSP_HLE_KERNEL_START_THREAD=16,
 PSP_HLE_MODULE_ID_BY_ADDRESS=17,
 PSP_HLE_INTR_SUSPEND=18,
 PSP_HLE_INTR_RESUME=19,
 PSP_HLE_PART_ALLOC=20, PSP_HLE_PART_HEAD=21, PSP_HLE_PART_FREE=22,
 PSP_HLE_STDIN=23, PSP_HLE_STDOUT=24, PSP_HLE_STDERR=25, PSP_HLE_IO_WRITE=26, PSP_HLE_SET_GPO=27
} PspHleService;
typedef struct { uint32_t syscall_id; PspHleService service; } PspHleBinding;
typedef struct {
 PspHleBinding entries[PSP_HLE_MAX_BINDINGS];
 size_t count;
 size_t last_binding_index; /* verified cache hint for repeated PSP HLE calls */
 struct PspIo *io; /* attached read-only disc filesystem */
 void (*stdio_write)(int fd, const uint8_t *bytes, size_t size);
 struct PspThreadCpu *threads; /* optional experimental thread services */
 uint64_t uptime_us;
 uint32_t compiled_sdk_version;
 uint32_t compiler_version;
 uint32_t guest_debug_output;
 /* Caller reserves this guest arena outside module and stack mappings. */
 uint32_t partition_low, partition_high, partition_next_uid;
 struct { uint32_t address, size, uid; } partition_blocks[16];
 int interrupts_suspended;
 uint32_t module_base, module_size, module_id;
 int trace_unresolved_imports; /* Opt in to trapping when an unresolved import is called. */
} PspHle;
void psp_hle_init(PspHle *hle);
int psp_hle_bind(PspHle *hle,uint32_t syscall_id,PspHleService service);
void psp_hle_attach_io(PspHle *hle,struct PspIo *io);
void psp_hle_attach_threads(PspHle *hle,struct PspThreadCpu *threads);
void psp_hle_advance_time(PspHle *hle,uint64_t delta_us);
/* 1: dispatched, 0: regular instruction; -3: unhandled trap. */
int psp_hle_step(PspHle *hle,PspCpu *cpu);
#endif
