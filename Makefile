# 3DS homebrew build; devkitARM and libctru required.
ifeq ($(strip $(DEVKITARM)),)
$(error DEVKITARM not set. Install devkitPro 3DS toolchain)
endif
include $(DEVKITARM)/3ds_rules
TARGET := psp3ds
BUILD := build
SOURCES := source
INCLUDES := include
ARCH := -march=armv6k -mtune=mpcore -mfloat-abi=hard -mtp=soft
CFLAGS := -O2 -Wall -Wextra -std=c11 -ffunction-sections $(ARCH) $(INCLUDE) -D__3DS__
LDFLAGS := -specs=3dsx.specs $(ARCH)
LIBS := -lctru -lm
LIBDIRS := $(CTRULIB)
export OUTPUT := $(CURDIR)/$(TARGET)
export VPATH := $(foreach dir,$(SOURCES),$(CURDIR)/$(dir))
export DEPSDIR := $(CURDIR)/$(BUILD)
export OFILES := $(addsuffix .o,$(basename $(notdir $(wildcard $(SOURCES)/*.c))))
export INCLUDE := $(foreach dir,$(INCLUDES),-I$(CURDIR)/$(dir)) $(foreach dir,$(LIBDIRS),-I$(dir)/include)
export LIBPATHS := $(foreach dir,$(LIBDIRS),-L$(dir)/lib)
.PHONY: all clean
all: $(BUILD)
	@$(MAKE) --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile
$(BUILD):
	@mkdir -p $@
clean:
	@rm -rf $(BUILD) $(TARGET).3dsx $(TARGET).elf
ifneq ($(BUILD),$(notdir $(CURDIR)))
else
$(OUTPUT).3dsx: $(OUTPUT).elf
$(OUTPUT).elf: $(OFILES)
-include $(DEPSDIR)/*.d
endif
