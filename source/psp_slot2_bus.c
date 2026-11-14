#include "psp_slot2_bus.h"
int psp_slot2_bus_init(PspGuestBus *bus,uint8_t *hot,
                       uint32_t hot_base,size_t hot_size,
                       const PspSlot2Result *probe,
                       volatile uint16_t *confirmed_ram,
                       uint32_t cold_guest_base,
                       PspPlatformBudget *selected){
 if(!bus||!probe||!selected)return -1;
 PspPlatformBudget budget;
 int expanded=psp_slot2_budget_select(probe,&budget);
 if(expanded<0)return -1;
 PspGuestWindow window;
 const PspGuestWindow *cold=0;
 if(expanded && confirmed_ram &&
    !psp_guest_window_init(&window,confirmed_ram,cold_guest_base,
                           budget.cold_data_budget,1,1))cold=&window;
 if(psp_guest_bus_init(bus,hot,hot_base,hot_size,cold))return -1;
 if(!cold && psp_platform_budget_choose(PSP_TARGET_DS,&budget))return -1;
 *selected=budget;
 return cold?1:0;
}
