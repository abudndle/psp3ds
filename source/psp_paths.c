#include "psp_paths.h"
#include <stdio.h>
#include <string.h>
int psp_game_save_dir(char *dst,size_t cap,const char *id){
 if(!dst || !id)return -1;
 size_t n=strlen(id);
 if(n<4 || n>24)return -1;
 for(size_t i=0;i<n;i++){
  unsigned char c=(unsigned char)id[i];
  if(!((c>='A'&&c<='Z')||(c>='0'&&c<='9')||c=='_'||c=='-'))return -1;
 }
 int written=snprintf(dst,cap,"psp/roms/save/%s",id);
 return written<0 || (size_t)written>=cap?-1:0;
}
