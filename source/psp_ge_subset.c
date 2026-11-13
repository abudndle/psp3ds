#include "psp_ge_subset.h"
#include <string.h>
/* PSP GE: high byte is opcode, low 24 bits are argument. Supported:
 * NOP(0x00), PRIM(0x04) sprite type 6 with two vertices,
 * FINISH(0x0f) and END(0x0c), in that order.
 * The rectangle list is a *synthetic predecoded fixture* and is NOT
 * read from PSP memory nor extracted from PSP vertex descriptors. */
int psp_ge_subset_translate(const uint32_t *words,size_t count,
                            const PspGeDecodedSprite *sprites,size_t sprite_count,
                            PspGeSubsetResult *out){
 if(out)memset(out,0,sizeof(*out));
 if(!out||!words||!count||count>PSP_GE_SUBSET_MAX_COMMANDS||
    sprite_count>PSP_GE_SUBSET_MAX_SPRITES||
    (sprite_count&&!sprites))return PSP_GE_SUBSET_BAD_ARGUMENT;
 PspGeSubsetResult staged;
 memset(&staged,0,sizeof staged);
 int finished=0,ended=0;
 size_t used=0;
 for(size_t i=0;i<count;i++){
  uint32_t word=words[i];
  unsigned opcode=word>>24;
  uint32_t arg=word&UINT32_C(0x00ffffff);
  if(ended)return PSP_GE_SUBSET_MALFORMED;
  if(opcode==0x00){ /* Only exact NOP with zero argument. */
   if(arg||finished)return PSP_GE_SUBSET_UNSUPPORTED;
  }else if(opcode==0x04){ /* PRIM: only sprite with two vertices. */
   if(finished)return PSP_GE_SUBSET_MALFORMED;
   if(arg!=UINT32_C(0x00060002))return PSP_GE_SUBSET_UNSUPPORTED;
   if(used>=sprite_count)return PSP_GE_SUBSET_MALFORMED;
   const PspGeDecodedSprite *s=&sprites[used++];
   if(!s->w||!s->h||s->x>=256u||s->y>=192u||
      (uint32_t)s->x+s->w>256u||(uint32_t)s->y+s->h>192u||
      s->bgr555>UINT16_C(0x7fff))return PSP_GE_SUBSET_MALFORMED;
   if(staged.batch.count>=PSP_SCENE_MAX_RECTS)return PSP_GE_SUBSET_OVERFLOW;
   PspSceneRect *r=&staged.batch.rects[staged.batch.count++];
   r->x=s->x;r->y=s->y;r->w=s->w;r->h=s->h;r->bgr555=s->bgr555;
  }else if(opcode==0x0f){ /* FINISH must be followed by END. */
   if(finished||arg)return PSP_GE_SUBSET_MALFORMED;
   finished=1;
  }else if(opcode==0x0c){ /* Terminates list; no trailing commands. */
   if(!finished||arg||used!=sprite_count)return PSP_GE_SUBSET_MALFORMED;
   ended=1;
  }else{
   /* Reject VTYPE, VADDR, BASE, JUMP, CALL, texture, blend, etc.
    * Never silently interpret unknown real PSP GE state. */
   return PSP_GE_SUBSET_UNSUPPORTED;
  }
 }
 if(!ended||!finished||!used)return PSP_GE_SUBSET_MALFORMED;
 staged.consumed_commands=(uint32_t)count;
 staged.consumed_sprites=(uint32_t)used;
 *out=staged;
 return PSP_GE_SUBSET_OK;
}
