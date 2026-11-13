#include "psp_paths.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>
int main(void){
 char p[128];
 assert(psp_game_save_dir(p,sizeof p,"NPUZ00292")==0);
 assert(strcmp(p,"psp/roms/save/NPUZ00292")==0);
 assert(psp_game_save_dir(p,sizeof p,"ULUS12345")==0);
 assert(psp_game_save_dir(p,sizeof p,"../GAME")<0);
 assert(psp_game_save_dir(p,sizeof p,"../../secret")<0);
 assert(psp_game_save_dir(p,sizeof p,"hello/world")<0);
 assert(psp_game_save_dir(p,sizeof p,"name with spaces")<0);
 assert(psp_game_save_dir(p,4,"NPUZ00292")<0);
 assert(psp_game_save_dir(p,sizeof p,"")<0);
 puts("PASS: game-specific save paths and directory traversal rejection");
 return 0;
}
