#ifndef PSP_CORE_H
#define PSP_CORE_H
#include <stdint.h>
#include <stddef.h>
typedef enum { PSP_FILE_UNKNOWN=0, PSP_FILE_PBP, PSP_FILE_ELF, PSP_FILE_CSO, PSP_FILE_ISO } PspFileType;
typedef struct { PspFileType type; uint32_t version; uint32_t offsets[8]; uint32_t entry; uint32_t machine; } PspFileInfo;
int psp_inspect(const uint8_t *data, size_t size, PspFileInfo *out);
#define PSP_RAM_SIZE (2u*1024u*1024u)
typedef struct { uint32_t gpr[32]; uint32_t fpr[32]; uint32_t pc, next_pc, hi, lo; uint32_t cp0_status, cp0_cause, cp0_epc, cp0_badvaddr; uint32_t last_trap_code; uint32_t ll_address; int ll_valid; int delay_slot; uint8_t *ram; size_t ram_size; uint32_t ram_base; int halted; int fault; } PspCpu;
void psp_cpu_init(PspCpu *cpu, uint8_t *ram, size_t ram_size, uint32_t ram_base, uint32_t start_pc);
int psp_cpu_step(PspCpu *cpu);
#endif
