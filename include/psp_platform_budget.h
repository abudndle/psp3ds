#ifndef PSP_PLATFORM_BUDGET_H
#define PSP_PLATFORM_BUDGET_H
#include <stdint.h>
#include <stddef.h>
typedef enum {
 PSP_TARGET_DS=0,
 PSP_TARGET_DS_SLOT2=1,
 PSP_TARGET_DSI=2,
 PSP_TARGET_OLD3DS=3,
 PSP_TARGET_NEW3DS=4,
 PSP_TARGET_COUNT=5
} PspTarget;
typedef enum {
 PSP_GRAPHICS_DS_TILES=0,
 PSP_GRAPHICS_3DS_SPRITES=1
} PspGraphicsPath;
/* Conservative *provisional software policy* caps, NOT measured free
 * system memory or actual commercial PSP resource requirements. */
typedef struct {
 PspTarget target;
 const char *name;
 uint32_t hot_ram_budget;
 uint32_t cold_data_budget;
 uint32_t cache_budget;
 uint32_t texture_budget;
 uint32_t io_budget;
 uint32_t frame_buffer_bytes;
 uint16_t width,height;
 uint8_t recommended_bpp;
 PspGraphicsPath graphics;
 int has_slot2;
} PspPlatformBudget;
int psp_platform_budget_choose(PspTarget target,PspPlatformBudget *out);
int psp_platform_budget_validate(const PspPlatformBudget *b);
int psp_platform_budget_frame_bytes(const PspPlatformBudget *b,
                                    uint8_t bpp,uint32_t *out);
#endif
