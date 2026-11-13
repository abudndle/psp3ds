# BlocksDS ARM9-only project, based on SDK template (CC0-1.0).
export BLOCKSDS ?= /opt/blocksds/core
export WONDERFUL_TOOLCHAIN ?= /opt/wonderful
NAME := psp3ds_nds
SOURCEDIRS := source
LIBS := -lmm9 -lnds9 -lc
LIBDIRS := $(BLOCKSDS)/libs/maxmod $(BLOCKSDS)/libs/libnds
ARM7ELF := $(BLOCKSDS)/sys/arm7/main_core/arm7_maxmod.elf
CC := $(WONDERFUL_TOOLCHAIN)/toolchain/gcc-arm-none-eabi/bin/arm-none-eabi-gcc
LD := $(CC)
BUILDDIR := build/$(NAME)
ELF := build/$(NAME).elf
ROM := $(NAME).nds
ARCH := -mthumb -mcpu=arm946e-s+nofp
SPECS := $(BLOCKSDS)/sys/crts/ds_arm9.specs
CFLAGS := -O2 -std=gnu11 -Wall -Wextra -D__NDS__ -D__BLOCKSDS__ -DARM9 $(ARCH) -specs=$(SPECS) -ffunction-sections -fdata-sections $(foreach d,$(LIBDIRS),-I$(d)/include) -I$(CURDIR)/../../include
LDFLAGS := $(ARCH) -specs=$(SPECS) -Wl,--gc-sections $(foreach d,$(LIBDIRS),-L$(d)/lib) -Wl,--start-group $(LIBS) -Wl,--end-group
SOURCES_C := $(wildcard source/*.c)
OBJECTS := $(addprefix $(BUILDDIR)/,$(notdir $(SOURCES_C:.c=.o)))
.PHONY: all clean
all: $(ROM)
$(ROM): $(ELF)
	$(BLOCKSDS)/tools/ndstool/ndstool -c $@ -7 $(ARM7ELF) -9 $(ELF)
$(ELF): $(OBJECTS)
	@mkdir -p $(@D)
	$(LD) $(OBJECTS) $(LDFLAGS) -o $@
$(BUILDDIR)/%.o: source/%.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -MMD -MP -c $< -o $@
-include $(OBJECTS:.o=.d)
clean:
	rm -rf build $(ROM)
