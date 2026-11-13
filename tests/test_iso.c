#include "psp_iso.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
int main(void){
 const char *path="test_psp3ds_disc.iso";
 FILE *f=fopen(path,"wb");assert(f);
 uint8_t sector[2048];
 for(unsigned j=0;j<18;j++){
  memset(sector,(int)j,sizeof sector);
  if(j==16)memcpy(sector+1,"CD001",5);
  assert(fwrite(sector,1,sizeof sector,f)==sizeof sector);
 }
 assert(fclose(f)==0);
 PspIso iso;PspPerf perf;psp_perf_init(&perf);
 assert(psp_iso_open(&iso,path)==0 && iso.sectors==18);
 uint8_t out[2048];
 assert(psp_iso_cached(&iso,&perf,3,out)==0 && out[0]==3);
 assert(psp_iso_cached(&iso,&perf,3,out)==1 && out[17]==3);
 assert(psp_iso_cached(&iso,&perf,16,out)==0 && !memcmp(out+1,"CD001",5));
 assert(psp_iso_cached(&iso,&perf,18,out)==-1);
 psp_iso_close(&iso);
 assert(psp_iso_read(&iso,3,out)==-1);
 assert(remove(path)==0);
 assert(psp_iso_open(&iso,path)==-1);
 puts("PASS: streamed ISO sectors, cache hits and disk bounds");return 0;
}
