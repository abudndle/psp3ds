#include "psp_catalog.h"
#include <dirent.h>
#include <string.h>
#include <sys/stat.h>
#include <stdint.h>
#include <stdio.h>
static char lower(char c){return c>='A'&&c<='Z'?(char)(c+('a'-'A')):c;}
static int suffix(const char *name,size_t len,const char *ext){
 size_t e=strlen(ext);
 if(len<=e)return 0;
 for(size_t i=0;i<e;i++)if(lower(name[len-e+i])!=ext[i])return 0;
 return 1;
}
static PspFileType extension(const char *name,size_t len){
 if(suffix(name,len,".iso"))return PSP_FILE_ISO;
 if(suffix(name,len,".cso"))return PSP_FILE_CSO;
 if(suffix(name,len,".pbp"))return PSP_FILE_PBP;
 return PSP_FILE_UNKNOWN;
}
int psp_catalog_scan(const char *dir,PspCatalogItem *items,size_t cap,size_t *count){
 if(count)*count=0;
 if(!dir||!items||!count||!cap)return -1;
 DIR *handle=opendir(dir);
 if(!handle)return -1;
 int status=0;struct dirent *entry;
 while((entry=readdir(handle))!=NULL){
  const char *name=entry->d_name;
  size_t n=strlen(name);
  if(n<5||n>=PSP_CATALOG_FILENAME_MAX||name[0]=='.')continue;
  PspFileType type=extension(name,n);
  if(type==PSP_FILE_UNKNOWN)continue;
  char path[512];
  int size=snprintf(path,sizeof path,"%s/%s",dir,name);
  if(size<0||(size_t)size>=sizeof path)continue;
  struct stat st;
  if(stat(path,&st)!=0||!S_ISREG(st.st_mode))continue;
  if(*count>=cap){status=-2;break;}
  size_t i=*count;
  while(i&&strcmp(items[i-1].filename,name)>0){items[i]=items[i-1];i--;}
  memcpy(items[i].filename,name,n+1);
  items[i].type=type;
  (*count)++;
 }
 closedir(handle);
 return status;
}
