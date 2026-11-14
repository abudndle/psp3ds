/* Read a user-provided, unencrypted PSP PRX-style ELF locally.
 * Uses the same public mapper as the Nintendo build. No private bytes are
 * embedded or uploaded and no guest MIPS instructions are executed. */
#include "psp_prx_image.h"
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>

#define PSP_PROBE_BASE UINT32_C(0x08800000)
#define PSP_PROBE_MAX_FILE (32u*1024u*1024u)
#define PSP_PROBE_MAX_RAM (32u*1024u*1024u)
static uint16_t u16(const uint8_t *p){return (uint16_t)(p[0]|(uint16_t)p[1]<<8);}
static uint32_t u32(const uint8_t *p){
 return (uint32_t)p[0]|(uint32_t)p[1]<<8|
        (uint32_t)p[2]<<16|(uint32_t)p[3]<<24;
}

int main(int argc,char **argv){
 if(argc!=2){
  fprintf(stderr,"Usage: %s /path/to/decrypted-EBOOT.elf\n",argv[0]);
  return 2;
 }
 FILE *f=fopen(argv[1],"rb");
 if(!f){perror("open PSP ELF");return 2;}
 if(fseek(f,0,SEEK_END)!=0){fclose(f);return 2;}
 long length=ftell(f);
 if(length<52 || (uint64_t)length>PSP_PROBE_MAX_FILE ||
    fseek(f,0,SEEK_SET)!=0){fclose(f);fprintf(stderr,"ELF file size invalid\n");return 2;}
 uint8_t *file=(uint8_t*)malloc((size_t)length);
 if(!file){fclose(f);return 2;}
 if(fread(file,1,(size_t)length,f)!=(size_t)length){
  fclose(f);free(file);fprintf(stderr,"ELF read failed\n");return 2;
 }
 fclose(f);
 if(memcmp(file,"\177ELF",4)!=0||file[4]!=1||file[5]!=1||
    u16(file+16)!=0xffa0u||u16(file+18)!=8u){
  fprintf(stderr,"Requires unencrypted PSP ELF ET_SCE_PRX (0xFFA0)\n");
  free(file);return 2;
 }
 const uint32_t phoff=u32(file+28);
 const unsigned entsize=u16(file+42),count=u16(file+44);
 if(entsize<32u||!count||count>128u||
    (uint64_t)phoff+(uint64_t)entsize*count>(uint64_t)length){
  free(file);fprintf(stderr,"Invalid program-header table\n");return 2;
 }
 uint64_t extent=0;
 for(unsigned i=0;i<count;i++){
  const uint8_t *ph=file+phoff+(size_t)i*entsize;
  if(u32(ph)!=1u)continue;
  const uint64_t end=(uint64_t)u32(ph+8)+u32(ph+20);
  if(end>extent)extent=end;
 }
 if(!extent||extent>PSP_PROBE_MAX_RAM){
  fprintf(stderr,"Guest image exceeds 32 MiB bound\n");free(file);return 2;
 }
 uint8_t *ram=(uint8_t*)calloc(1,(size_t)extent);
 if(!ram){free(file);return 2;}
 PspCpu cpu;
 psp_cpu_init(&cpu,ram,(size_t)extent,PSP_PROBE_BASE,PSP_PROBE_BASE);
 PspPrxImageResult image;
 int result=psp_prx_image_map_relocated(file,(size_t)length,&cpu,&image);
 if(result==PSP_PRX_IMAGE_OK){
  printf("PSP PRX mapping: PASS\n");
  printf("Guest entry: 0x%08lx\n",(unsigned long)image.entry);
  printf("Guest image: %lu bytes\n",(unsigned long)image.image_bytes);
  printf("Loaded segments: %u\n",(unsigned)image.load_segments);
  printf("Relocation streams: %u\n",(unsigned)image.relocation_streams);
  puts("PSP kernel/HLE imports and gameplay were NOT executed.");
 }else fprintf(stderr,"PSP PRX mapping: FAILED (error %d)\n",result);
 free(ram);free(file);
 return result==PSP_PRX_IMAGE_OK?0:1;
}
