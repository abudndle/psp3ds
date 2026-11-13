#ifndef PSP_PATHS_H
#define PSP_PATHS_H
#include <stddef.h>
/* Game-specific relative save directory; does not mutate filesystem. */
int psp_game_save_dir(char *dst,size_t cap,const char *disc_id);
#endif
