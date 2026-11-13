#include "psp_catalog.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
static void create(const char *path){FILE *f=fopen(path,"wb");assert(f);fputc(0,f);assert(!fclose(f));}
int main(void){
 const char *dir="test_psp_catalog_dir";
 (void)mkdir(dir,0700);(void)mkdir("test_psp_catalog_dir/save",0700);
 create("test_psp_catalog_dir/BGame.CSO");
 create("test_psp_catalog_dir/AGame.iso");
 create("test_psp_catalog_dir/Homebrew.pbp");
 create("test_psp_catalog_dir/readme.txt");
 PspCatalogItem items[4];size_t count=999;
 assert(psp_catalog_scan(dir,items,4,&count)==0&&count==3);
 assert(strcmp(items[0].filename,"AGame.iso")==0&&items[0].type==PSP_FILE_ISO);
 assert(strcmp(items[1].filename,"BGame.CSO")==0&&items[1].type==PSP_FILE_CSO);
 assert(strcmp(items[2].filename,"Homebrew.pbp")==0&&items[2].type==PSP_FILE_PBP);
 assert(psp_catalog_scan(dir,items,2,&count)==-2&&count==2);
 assert(psp_catalog_scan("missing_psp_rom_directory",items,4,&count)==-1&&count==0);
 remove("test_psp_catalog_dir/BGame.CSO");remove("test_psp_catalog_dir/AGame.iso");
 remove("test_psp_catalog_dir/Homebrew.pbp");remove("test_psp_catalog_dir/readme.txt");
 rmdir("test_psp_catalog_dir/save");rmdir(dir);
 puts("PASS: ROM catalog filters and sorts files, never traverses save folder");
 return 0;
}
