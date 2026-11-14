#ifndef PSP_HLE_H
#define PSP_HLE_H
#include "psp_core.h"
#include <stddef.h>
#include <stdint.h>
struct PspIo;
struct PspThreadCpu;
/* PSP syscall numbers are assigned by module import resolution; no global fixed IDs. */
#define PSP_HLE_MAX_BINDINGS 64u
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
 /* PSP SysMemUserForUser NID 0x358CA1BB: verified SDK-version setter. */
 PSP_HLE_SET_COMPILED_SDK_VERSION=12
} PspHleService;
typedef struct { uint32_t syscall_id; PspHleService service; } PspHleBinding;
typedef struct {
 PspHleBinding entries[PSP_HLE_MAX_BINDINGS];
 size_t count;
 size_t last_binding_index; /* verified cache hint for repeated PSP HLE calls */
 struct PspIo *io; /* attached read-only disc filesystem */
 struct PspThreadCpu *threads; /* optional experimental thread services */
 uint64_t uptime_us;
 uint32_t compiled_sdk_version; /* per-module SDK configuration */
} PspHle;
void psp_hle_init(PspHle *hle);
int psp_hle_bind(PspHle *hle,uint32_t syscall_id,PspHleService service);
void psp_hle_attach_io(PspHle *hle,struct PspIo *io);
void psp_hle_attach_threads(PspHle *hle,struct PspThreadCpu *threads);
void psp_hle_advance_time(PspHle *hle,uint64_t delta_us);
/* 1: dispatched, 0: regular instruction; -3: unhandled trap. */
int psp_hle_step(PspHle *hle,PspCpu *cpu);
#endif
