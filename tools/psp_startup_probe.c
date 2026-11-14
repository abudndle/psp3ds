/* Local and private PSP entry checkpoint for a user-owned, decrypted ELF.
 * Host-only: NEVER upload copyrighted game images to public CI or Git.
 * No graphics/audio/kernel service is faked; stop at first real blocker. */
#include "psp_guarded_prx.h"
#include "psp_startup_trace.h"
#include "psp_module.h"
#include "psp_3ds_memory.h"
#include "psp_imports.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define HOST_ELF_FILE_LIMIT (8u*1024u*1024u)
#define PSP_GUEST_BASE UINT32_C(0x08800000)
#define BOOT_STACK_BYTES (256u*1024u)

int main(int argc,char **argv){
 if(argc<2||argc>4){
  fprintf(stderr,"Usage: %s decrypted-EBOOT.elf [max-steps=4096] [--link-sdk-version]\n",argv[0]);
  return 2;
 }
 unsigned steps=4096u;
 int link_sdk=0,steps_set=0;
 for(int i=2;i<argc;i++){
  if(strcmp(argv[i],"--link-sdk-version")==0){
   if(link_sdk)return 2;
   link_sdk=1;
   continue;
  }
  if(steps_set)return 2;
  char *end=NULL;
  unsigned long parsed=strtoul(argv[i],&end,10);
  if(!end||end==argv[i]||*end||parsed<1u||
     parsed>PSP_STARTUP_TRACE_MAX_STEPS){
   fprintf(stderr,"Instruction cap must be in 1..8192\n");
   return 2;
  }
  steps_set=1;
  steps=(unsigned)parsed;
 }
 FILE *f=fopen(argv[1],"rb");
 if(!f){perror("read-only ELF open");return 2;}
 if(fseek(f,0,SEEK_END)!=0){fclose(f);return 2;}
 long file_len=ftell(f);
 if(file_len<52||file_len>HOST_ELF_FILE_LIMIT||fseek(f,0,SEEK_SET)!=0){
  fprintf(stderr,"ELF length invalid or exceeds 8 MiB safety cap\n");
  fclose(f);return 2;
 }

 /* This is a deliberately generous HOST POLICY FIXTURE, not a claim
  * about free memory on a physical New 3DS. Native probes query the OS. */
 Psp3dsMemoryPlan plan;
 PspMemoryGuardPolicy policy;
 PspMemoryGuard guard={0};
 int result=1;
 if(psp_3ds_memory_plan(PSP_TARGET_NEW3DS,
       124u*PSP_3DS_MIB,110u*PSP_3DS_MIB,&plan)!=0 ||
    psp_memory_guard_policy_3ds(&plan,&policy)!=0 ||
    psp_memory_guard_init(&guard,&policy,NULL,NULL,NULL)!=0){
  fprintf(stderr,"Host test memory policy unavailable\n");
  fclose(f);return 1;
 }
 void *source=NULL;
 PspGuardedPrx loaded={0};
 if(psp_memory_guard_acquire(&guard,PSP_MEMORY_GUEST,
                            (size_t)file_len,&source)!=0){
  fprintf(stderr,"Source ELF exceeds bounded host fixture memory\n");
  goto cleanup;
 }
 if(fread(source,1,(size_t)file_len,f)!=(size_t)file_len){
  fprintf(stderr,"Unable to read entire ELF\n");goto cleanup;
 }
 if(psp_guarded_prx_load_with_reserve(&guard,(const uint8_t*)source,
             (size_t)file_len,PSP_GUEST_BASE,BOOT_STACK_BYTES,&loaded)!=0){
  fprintf(stderr,"PRX relocation or bounded guest allocation failed\n");
  goto cleanup;
 }

 PspModuleInfo module;
 PspPrxImportInventory inventory;
 PspStartupTraceConfig cfg;
 if(psp_module_discover((const uint8_t*)source,(size_t)file_len,
                         &loaded.cpu,&module)!=0 ||
    psp_prx_import_inventory(&loaded.cpu,module.import_top,
                             module.import_end,&inventory)!=0 ||
    psp_startup_trace_config_elf((const uint8_t*)source,
                       (size_t)file_len,PSP_GUEST_BASE,
                       loaded.allocated_bytes,steps,&cfg)!=0){
  fprintf(stderr,"Loaded PRX module/import metadata unsupported\n");
  goto cleanup;
 }
 const uint32_t image_bytes=loaded.image.image_bytes;
 const uint32_t import_count=inventory.function_count;
 const uint32_t lib_count=inventory.library_count;
 cfg.imports=&inventory;
 PspHle hle;
 psp_hle_init(&hle);
 if(link_sdk){
  /* Enable ONLY the verified NID reached at the first startup checkpoint.
   * All other imports remain unlinked and fail explicitly in the tracer. */
  const uint32_t required_nid=UINT32_C(0x358ca1bb);
  PspImport selected={0};
  unsigned matches=0;
  for(unsigned i=0;i<inventory.library_count;i++){
   const PspPrxLibraryInventory *lib=&inventory.libraries[i];
   if(strcmp(lib->name,"SysMemUserForUser")!=0)continue;
   for(unsigned j=0;j<lib->function_count;j++){
    uint64_t addr=(uint64_t)lib->nid_table+(uint64_t)j*4u;
    if(addr<loaded.cpu.ram_base || addr+4u>
       (uint64_t)loaded.cpu.ram_base+loaded.cpu.ram_size)goto cleanup;
    const uint8_t *p=loaded.cpu.ram+(size_t)(addr-loaded.cpu.ram_base);
    const uint32_t nid=(uint32_t)p[0]|(uint32_t)p[1]<<8|
                       (uint32_t)p[2]<<16|(uint32_t)p[3]<<24;
    if(nid!=required_nid)continue;
    selected=(PspImport){"SysMemUserForUser",required_nid,
                          lib->stub_table+8u*j};
    matches++;
   }
  }
  if(matches!=1u ||
     psp_imports_link_known(&loaded.cpu,&hle,&selected,1u)!=0){
   fprintf(stderr,"Optional SDK-version NID unavailable or duplicate\n");
   goto cleanup;
  }
  cfg.hle=&hle;
 }
 if(psp_startup_prepare_cpu(&loaded.cpu,image_bytes,
                            BOOT_STACK_BYTES,module.gp)!=0){
  fprintf(stderr,"Unable to reserve a diagnostic guest stack\n");
  goto cleanup;
 }
 /* All file-resident PRX metadata has been copied/decoded; release its
  * 2.9-MiB file image BEFORE the interpreter starts. */
 if(psp_memory_guard_release(&guard,source)!=0)goto cleanup;
 source=NULL;
 PspStartupTraceReport trace;
 if(psp_startup_trace_run(&loaded.cpu,&cfg,&trace)!=0){
  fprintf(stderr,"Startup trace rejected ELF or guest configuration\n");
  goto cleanup;
 }
 printf("ELF mapped image: %lu bytes\n",(unsigned long)image_bytes);
 printf("Module imports: %lu in %lu libraries\n",
        (unsigned long)import_count,(unsigned long)lib_count);
 printf("Startup checkpoint: %s\n",psp_startup_stop_name(trace.stop));
 printf("Entry PC: 0x%08lx\n",(unsigned long)trace.start_pc);
 printf("Stop PC: 0x%08lx\n",(unsigned long)trace.stop_pc);
 printf("Executed instructions: %lu (cap %u)\n",
        (unsigned long)trace.executed,steps);
 printf("PC path fingerprint: %08lx\n",
        (unsigned long)trace.pc_fingerprint);
 printf("First unsupported opcode class: %u\n",
        (unsigned)trace.major_opcode);
 if(link_sdk)
  printf("SDK version recorded by HLE: 0x%08lx\n",
         (unsigned long)hle.compiled_sdk_version);
 if(trace.stop==PSP_STARTUP_UNLINKED_IMPORT){
  printf("Import library: %s\n",trace.import_library);
  printf("Import NID: 0x%08lx\n",(unsigned long)trace.import_nid);
  printf("Matching implemented NID: %s (STILL NOT LINKED)\n",
         trace.import_known_hle?"yes":"no");
 }else if(trace.stop==PSP_STARTUP_SYSCALL){
  printf("Unbound syscall ID: %lu\n",
         (unsigned long)trace.syscall_id);
 }
 puts("Checkpoint only. PSP gameplay, graphics and audio NOT booted.");
 result=0;
cleanup:
 if(source)psp_memory_guard_release(&guard,source);
 if(loaded.guest_ram)psp_guarded_prx_unload(&guard,&loaded);
 PspMemoryGuardReport memory={0};
 if(psp_memory_guard_close(&guard,&memory)!=0)result=1;
 if(memory.reclaimed_allocations){
  fprintf(stderr,"Unexpected tracked allocations were reclaimed at exit\n");
  result=1;
 }
 printf("Host peak tracked RAM: %lu bytes; remaining: %lu bytes\n",
        (unsigned long)memory.peak_bytes,
        (unsigned long)memory.end_outstanding_bytes);
 fclose(f);
 return result;
}
