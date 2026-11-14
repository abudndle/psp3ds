#include "psp_startup_trace.h"
#include "psp_imports.h"
#include <stdint.h>
#include <string.h>

#define PSP_TRACE_ELF_TYPE 0xffa0u
#define PSP_TRACE_PT_LOAD 1u

static uint16_t rd16(const uint8_t *p){
 return (uint16_t)(p[0]|(uint16_t)p[1]<<8);
}
static uint32_t rd32(const uint8_t *p){
 return (uint32_t)p[0]|(uint32_t)p[1]<<8|
        (uint32_t)p[2]<<16|(uint32_t)p[3]<<24;
}
static const uint8_t *guest(const PspCpu *c,uint64_t address,size_t bytes){
 if(!c||!c->ram||address<c->ram_base||address>UINT32_MAX)return NULL;
 const uint64_t off=address-c->ram_base;
 if(off>c->ram_size||bytes>c->ram_size-(size_t)off)return NULL;
 return c->ram+(size_t)off;
}
int psp_startup_prepare_cpu(PspCpu *cpu,uint32_t mapped_image,
                            uint32_t stack_bytes,uint32_t gp){
 if(!cpu||!cpu->ram || cpu->ram_size>UINT32_MAX ||
    stack_bytes<PSP_STARTUP_MIN_STACK ||
    stack_bytes>PSP_STARTUP_MAX_STACK ||
    (stack_bytes&15u) || mapped_image>cpu->ram_size ||
    stack_bytes>cpu->ram_size-mapped_image ||
    (uint64_t)cpu->ram_base+cpu->ram_size>UINT32_MAX)return -1;
 const uint64_t top=(uint64_t)cpu->ram_base+cpu->ram_size;
 const uint32_t sp=(uint32_t)((top-16u)&~UINT64_C(15));
 if(sp<(uint64_t)cpu->ram_base+mapped_image)return -1;
 cpu->gpr[29]=sp; /* stack pointer */
 cpu->gpr[28]=gp; /* module global pointer */
 cpu->gpr[4]=0;   /* synthetic argc */
 cpu->gpr[5]=0;   /* synthetic argv */
 cpu->gpr[31]=0;  /* return address: returning stops at outside code */
 return 0;
}
int psp_startup_trace_config_elf(const uint8_t *elf,size_t size,
                                 uint32_t base,uint32_t capacity,
                                 uint32_t budget,
                                 PspStartupTraceConfig *out){
 if(out)memset(out,0,sizeof(*out));
 if(!out||!elf||size<52||!capacity||!budget||
    budget>PSP_STARTUP_TRACE_MAX_STEPS||
    (uint64_t)base+capacity>UINT32_MAX ||
    memcmp(elf,"\177ELF",4)!=0 || elf[4]!=1 || elf[5]!=1 ||
    elf[6]!=1 || rd16(elf+16)!=PSP_TRACE_ELF_TYPE ||
    rd16(elf+18)!=8 || rd16(elf+40)<52)return -1;
 const uint32_t phoff=rd32(elf+28);
 const unsigned phsize=rd16(elf+42),count=rd16(elf+44);
 if(!count || count>128u || phsize<32u ||
    (uint64_t)phoff+(uint64_t)phsize*count>size)return -1;
 uint8_t found=0;
 for(unsigned i=0;i<count;i++){
  const uint8_t *ph=elf+phoff+(size_t)i*phsize;
  if(rd32(ph)!=PSP_TRACE_PT_LOAD || !(rd32(ph+24)&1u))continue;
  const uint32_t off=rd32(ph+4),vaddr=rd32(ph+8),
                 filesz=rd32(ph+16),memsz=rd32(ph+20);
  if(!filesz || filesz>memsz ||
     (uint64_t)off+filesz>size ||
     (uint64_t)vaddr+memsz>capacity ||
     (uint64_t)base+vaddr+filesz>UINT32_MAX ||
     found==PSP_STARTUP_TRACE_MAX_RANGES){
   memset(out,0,sizeof(*out));return -1;
  }
  PspStartupExecutableRange *range=&out->executable[found++];
  range->first=base+vaddr;
  range->one_past_last=base+vaddr+filesz;
 }
 if(!found)return -1;
 out->executable_count=found;
 out->instruction_cap=budget;
 return 0;
}
static int executable(const PspStartupTraceConfig *c,uint32_t pc){
 for(unsigned i=0;i<c->executable_count;i++)
  if(pc>=c->executable[i].first &&
     (uint64_t)pc+4u<=c->executable[i].one_past_last)return 1;
 return 0;
}
static int at_import(const PspCpu *cpu,const PspPrxImportInventory *inv,
                     uint32_t pc,PspStartupTraceReport *out){
 if(!inv)return 0;
 if(inv->library_count>PSP_PRX_INVENTORY_MAX_LIBRARIES ||
    inv->function_count>PSP_PRX_INVENTORY_MAX_FUNCTIONS)return -1;
 unsigned total=0;
 for(unsigned i=0;i<inv->library_count;i++){
  const PspPrxLibraryInventory *lib=&inv->libraries[i];
  if(!lib->function_count ||
     lib->function_count>PSP_PRX_INVENTORY_MAX_FUNCTIONS-total)return -1;
  const uint64_t first=lib->stub_table;
  const uint64_t last=first+(uint64_t)lib->function_count*8u;
  if(pc>=first && pc<last){
   const uint32_t index=(uint32_t)((pc-first)/8u);
   const uint64_t nid_addr=(uint64_t)lib->nid_table+4u*index;
   const uint8_t *nid=guest(cpu,nid_addr,4u);
   const void *name_end=memchr(lib->name,0,PSP_PRX_MAX_NAME);
   if(!nid||!name_end||lib->name[0]==0)return -1;
   out->import_nid=rd32(nid);
   memcpy(out->import_library,lib->name,PSP_PRX_MAX_NAME);
   PspHleService service;
   const int known=psp_imports_known_service(lib->name,out->import_nid,
                                             &service);
   if(known<0)return -1;
   out->import_known_hle=(uint8_t)(known==1);
   return 1;
  }
  total+=lib->function_count;
 }
 if(total!=inv->function_count)return -1;
 return 0;
}
/* Only an exact NID that has been atomically bound to a valid syscall
 * stub may pass the import boundary. This checks both the actual guest
 * stub and the HLE registry, never assuming that a recognized NID alone
 * means the service is initialized or emulated correctly. */
static int linked_import(const PspCpu *cpu,const PspHle *hle,
                         const PspPrxImportInventory *imports,
                         const PspStartupTraceReport *result,uint32_t pc){
 if(!cpu||!hle||!result||!result->import_known_hle)return 0;
 if(!result->import_library[0])return 0;
 PspHleService service;
 if(psp_imports_known_service(result->import_library,
          result->import_nid,&service)!=1)return 0;
 if(!cpu->ram||!hle->count)return 0;
 if(!imports)return 0;
 uint64_t stub_address=0;
 for(unsigned i=0;i<imports->library_count;i++){
  const PspPrxLibraryInventory *lib=&imports->libraries[i];
  const uint64_t first=lib->stub_table;
  const uint64_t end=first+(uint64_t)lib->function_count*8u;
  if(pc>=first && pc<end){
   stub_address=first+(((uint64_t)pc-first)/8u)*8u;
   break;
  }
 }
 if(!stub_address)return 0;
 const uint8_t *stub=guest(cpu,stub_address,8u);
 if(!stub)return 0;
 const uint32_t jump=rd32(stub),syscall=rd32(stub+4u);
 if(jump!=UINT32_C(0x03e00008) || (syscall&63u)!=12u)return 0;
 const uint32_t id=syscall>>6u;
 for(size_t i=0;i<hle->count;i++)
  if(hle->entries[i].syscall_id==id &&
     hle->entries[i].service==service)return 1;
 return 0;
}
int psp_startup_trace_run(PspCpu *cpu,
                          const PspStartupTraceConfig *config,
                          PspStartupTraceReport *out){
 if(out)memset(out,0,sizeof(*out));
 if(!cpu||!out||!cpu->ram||!config||
    (config->hle && config->hle->count>PSP_HLE_MAX_BINDINGS) ||
    !config->instruction_cap||
    config->instruction_cap>PSP_STARTUP_TRACE_MAX_STEPS||
    !config->executable_count ||
    config->executable_count>PSP_STARTUP_TRACE_MAX_RANGES)return -1;
 for(unsigned i=0;i<config->executable_count;i++)
  if(config->executable[i].one_past_last<=config->executable[i].first ||
     !guest(cpu,config->executable[i].first,
       (size_t)(config->executable[i].one_past_last-
                config->executable[i].first)))return -1;
 out->start_pc=cpu->pc;
 out->pc_fingerprint=UINT32_C(2166136261);
 for(unsigned i=0;i<config->instruction_cap;i++){
  const uint32_t pc=cpu->pc;
  out->stop_pc=pc;
  out->next_pc=cpu->next_pc;
  if(!executable(config,pc)){
   out->stop=PSP_STARTUP_OUTSIDE_CODE;
   return 0;
  }
  const uint8_t *pointer=guest(cpu,pc,4u);
  if(!pointer)return -1; /* fully checked ranges above */
  const uint32_t instr=rd32(pointer);
  out->major_opcode=(uint8_t)(instr>>26);
  out->function_code=(uint8_t)(instr&63u);
  /* Stop at real unresolved firmware import stubs. Do not patch them
   * into successful fake HLE calls or jump into kernel addresses. */
  const int imported=at_import(cpu,config->imports,pc,out);
  if(imported<0)return -1;
  if(imported && !linked_import(cpu,config->hle,config->imports,
                                 out,pc)){
   out->stop=PSP_STARTUP_UNLINKED_IMPORT;
   return 0;
  }
  /* Fingerprint executed PC path only; no copyrighted opcode bytes. */
  out->pc_fingerprint=(out->pc_fingerprint^pc)*UINT32_C(16777619);
  const int status=config->hle?
    psp_hle_step(config->hle,cpu):psp_cpu_step(cpu);
  out->interpreter_status=status;
  if(status==0 || status==1){
   out->executed++;
   if(status==1)out->handled_hle_traps++;
   /* A linked import was successfully handled; do not leave its
    * identity in a subsequent unrelated budget/fault checkpoint. */
   if(imported){
    out->import_nid=0;
    out->import_library[0]=0;
    out->import_known_hle=0;
   }
   continue;
  }
  if(status==-2)out->stop=PSP_STARTUP_MEMORY_FAULT;
  else if(status==-3){
   const uint32_t exception=(cpu->cp0_cause>>2u)&31u;
   if(exception==8u){
    out->stop=PSP_STARTUP_SYSCALL;
    out->syscall_id=cpu->last_trap_code;
   }else out->stop=PSP_STARTUP_EXCEPTION;
  }else if(status==-1){
   if(out->major_opcode==18u)out->stop=PSP_STARTUP_VFPU_REQUIRED;
   else if(out->major_opcode==17u ||
           out->major_opcode==49u ||
           out->major_opcode==53u ||
           out->major_opcode==57u ||
           out->major_opcode==61u)
    out->stop=PSP_STARTUP_FPU_REQUIRED;
   else out->stop=PSP_STARTUP_UNSUPPORTED_MIPS;
  }else out->stop=PSP_STARTUP_EXCEPTION;
  return 0;
 }
 out->stop=PSP_STARTUP_BUDGET;
 out->stop_pc=cpu->pc;
 out->next_pc=cpu->next_pc;
 return 0;
}
const char *psp_startup_stop_name(PspStartupStop stop){
 switch(stop){
  case PSP_STARTUP_BUDGET:return "instruction_budget_reached";
  case PSP_STARTUP_UNLINKED_IMPORT:return "unresolved_firmware_import";
  case PSP_STARTUP_SYSCALL:return "unbound_syscall";
  case PSP_STARTUP_UNSUPPORTED_MIPS:return "unsupported_mips_opcode";
  case PSP_STARTUP_FPU_REQUIRED:return "fpu_instruction_required";
  case PSP_STARTUP_VFPU_REQUIRED:return "vfpu_instruction_required";
  case PSP_STARTUP_MEMORY_FAULT:return "guest_memory_fault";
  case PSP_STARTUP_OUTSIDE_CODE:return "outside_executable_code";
  case PSP_STARTUP_EXCEPTION:return "guest_exception";
  default:return "invalid";
 }
}
