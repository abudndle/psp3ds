#include "psp_cso.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <zlib.h>
static void w32(unsigned char *p,uint32_t x){for(int i=0;i<4;i++)p[i]=(unsigned char)(x>>(8*i));}
static void w64(unsigned char *p,uint64_t x){w32(p,(uint32_t)x);w32(p+4,(uint32_t)(x>>32));}
static void make_fixture(const char *name,int bad_version) {
 FILE *f=fopen(name,"wb");assert(f);
 unsigned char h[36]={0};memcpy(h,"CISO",4);w32(h+4,24);w64(h+8,4096);
 w32(h+16,2048);h[20]=(unsigned char)(bad_version?2:1);
 unsigned char plain[2048],source[2048],packed[4096];
 memset(plain,0x5a,sizeof plain);
 for(unsigned i=0;i<2048;i++)source[i]=(unsigned char)((i*i+13*i)%251);
 z_stream z={0};assert(deflateInit2(&z,Z_BEST_SPEED,Z_DEFLATED,-MAX_WBITS,8,Z_DEFAULT_STRATEGY)==Z_OK);
 z.next_in=plain;z.avail_in=sizeof plain;z.next_out=packed;z.avail_out=sizeof packed;
 assert(deflate(&z,Z_FINISH)==Z_STREAM_END);
 size_t compressed=z.total_out;deflateEnd(&z);
 w32(h+24,36);w32(h+28,(uint32_t)(36+compressed)|0x80000000u);
 w32(h+32,(uint32_t)(36+compressed+2048));
 assert(fwrite(h,1,sizeof h,f)==sizeof h);
 assert(fwrite(packed,1,compressed,f)==compressed);
 assert(fwrite(source,1,sizeof source,f)==sizeof source);
 assert(!fclose(f));
}
int main(void) {
 const char *filename="test_psp3ds_cso_image.cso";
 make_fixture(filename,0);
 PspCso cso;PspPerf perf;psp_perf_init(&perf);
 assert(psp_cso_open(&cso,filename)==0 && cso.sectors==2);
 uint8_t out[2048];memset(out,0,sizeof out);
 assert(psp_cso_cached(&cso,&perf,0,out)==0);
 for(unsigned i=0;i<2048;i++)assert(out[i]==0x5a);
 assert(psp_cso_cached(&cso,&perf,0,out)==1);
 assert(psp_cso_cached(&cso,&perf,1,out)==0);
 for(unsigned i=0;i<2048;i++)assert(out[i]==(uint8_t)((i*i+13*i)%251));
 assert(psp_cso_cached(&cso,&perf,2,out)==-1);
 assert(psp_cso_read(&cso,2,out)==-1);
 psp_cso_close(&cso);
 assert(psp_cso_read(&cso,0,out)==-1);
 /* Some commercial CSO v1 images write zero in header_size while the
  * first index entry still starts at byte 24. Accept this common variant. */
 make_fixture(filename,0);
 FILE *zero_header=fopen(filename,"r+b");assert(zero_header);
 unsigned char zero[4]={0};
 assert(fseek(zero_header,4,SEEK_SET)==0 && fwrite(zero,1,4,zero_header)==4);
 assert(fclose(zero_header)==0);
 assert(psp_cso_open(&cso,filename)==0);
 assert(psp_cso_read(&cso,0,out)==0 && out[0]==0x5a);
 psp_cso_close(&cso);
 /* Invalid version is rejected instead of reading v2 as v1. */
 make_fixture(filename,1);assert(psp_cso_open(&cso,filename)==-1);
 make_fixture(filename,0);
 FILE *f=fopen(filename,"r+b");assert(f);
 unsigned char invalid[4];w32(invalid,0x7fffffffU);
 assert(!fseek(f,28,SEEK_SET));assert(fwrite(invalid,1,4,f)==4);fclose(f);
 assert(psp_cso_open(&cso,filename)==0);
 assert(psp_cso_read(&cso,0,out)==-1);
 psp_cso_close(&cso);
 assert(!remove(filename));
 puts("PASS: CSO v1 raw DEFLATE, plain sectors, LRU and malformed indexes");
 return 0;
}
