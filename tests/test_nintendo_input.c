#include "psp_nintendo_input.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>

static void check_buttons(PspTarget target) {
 PspNintendoCtrl state;
 assert(psp_nintendo_ctrl_sample(target,0,0,0,&state)==0);
 assert(state.buttons==0&&state.lx==128&&state.ly==128);
 const uint32_t physical=PSP_NINTENDO_KEY_A|PSP_NINTENDO_KEY_B|
  PSP_NINTENDO_KEY_X|PSP_NINTENDO_KEY_Y|
  PSP_NINTENDO_KEY_UP|PSP_NINTENDO_KEY_DOWN|
  PSP_NINTENDO_KEY_LEFT|PSP_NINTENDO_KEY_RIGHT|
  PSP_NINTENDO_KEY_SELECT|PSP_NINTENDO_KEY_START|
  PSP_NINTENDO_KEY_L|PSP_NINTENDO_KEY_R;
 const uint32_t psp=PSP_CTRL_CIRCLE|PSP_CTRL_CROSS|
  PSP_CTRL_TRIANGLE|PSP_CTRL_SQUARE|PSP_CTRL_UP|
  PSP_CTRL_DOWN|PSP_CTRL_LEFT|PSP_CTRL_RIGHT|
  PSP_CTRL_SELECT|PSP_CTRL_START|PSP_CTRL_LTRIGGER|PSP_CTRL_RTRIGGER;
 assert(psp_nintendo_ctrl_sample(target,physical,0,0,&state)==0);
 assert(state.buttons==psp);
 assert(state.lx==128&&state.ly==128); /* opposite D-pad pairs cancel */
 assert(psp_nintendo_ctrl_sample(target,0xffffffffu,0,0,&state)==0);
 assert(state.buttons==psp); /* high 3DS cpad and C-stick aliases ignored */
}
int main(void) {
 const PspTarget targets[]={PSP_TARGET_DS,PSP_TARGET_DS_SLOT2,PSP_TARGET_DSI,
                            PSP_TARGET_OLD3DS,PSP_TARGET_NEW3DS};
 for(unsigned i=0;i<sizeof targets/sizeof targets[0];++i)check_buttons(targets[i]);
 PspNintendoCtrl s;
 for(int target=PSP_TARGET_DS;target<=PSP_TARGET_DSI;++target){
  assert(psp_nintendo_ctrl_sample((PspTarget)target,
   PSP_NINTENDO_KEY_RIGHT|PSP_NINTENDO_KEY_UP,9999,-9999,&s)==0);
  assert(s.lx==255&&s.ly==0);
  assert((s.buttons&(PSP_CTRL_RIGHT|PSP_CTRL_UP))==(PSP_CTRL_RIGHT|PSP_CTRL_UP));
  assert(psp_nintendo_ctrl_sample((PspTarget)target,
   PSP_NINTENDO_KEY_LEFT|PSP_NINTENDO_KEY_DOWN,0,0,&s)==0);
  assert(s.lx==0&&s.ly==255);
 }
 for(int target=PSP_TARGET_OLD3DS;target<=PSP_TARGET_NEW3DS;++target){
  assert(psp_nintendo_ctrl_sample((PspTarget)target,PSP_NINTENDO_KEY_UP,
   156,156,&s)==0);
  assert(s.buttons==PSP_CTRL_UP&&s.lx==255&&s.ly==0);
  assert(psp_nintendo_ctrl_sample((PspTarget)target,0,-156,-156,&s)==0);
  assert(s.lx==0&&s.ly==255);
  assert(psp_nintendo_ctrl_sample((PspTarget)target,0,999999,-999999,&s)==0);
  assert(s.lx==255&&s.ly==255);
  assert(psp_nintendo_ctrl_sample((PspTarget)target,
   PSP_NINTENDO_KEY_UP|PSP_NINTENDO_KEY_RIGHT,0,0,&s)==0);
  assert(s.buttons==(PSP_CTRL_UP|PSP_CTRL_RIGHT));
  assert(s.lx==128&&s.ly==128); /* D-pad does not fake 3DS analog */
 }
 s.buttons=0xdeadbeefu; s.lx=1; s.ly=255;
 assert(psp_nintendo_ctrl_sample(PSP_TARGET_COUNT,0,0,0,&s)==-1);
 assert(s.buttons==0&&s.lx==0&&s.ly==0);
 assert(psp_nintendo_ctrl_sample(PSP_TARGET_DS,0,0,0,NULL)==-1);
 puts("PASS: upstream PSP Ctrl masks and integer analog on DS, Slot-2, DSi and both 3DS families");
 return 0;
}
