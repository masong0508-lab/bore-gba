# Needs devkitARM (arm-none-eabi-gcc + gbafix on PATH). The GitHub workflow uses the devkitpro/devkitarm image.
TARGET  := bore
CC      := arm-none-eabi-gcc
OBJCOPY := arm-none-eabi-objcopy
ARCH    := -mthumb -mthumb-interwork
CFLAGS  := $(ARCH) -O2 -Wall -fno-strict-aliasing -ffunction-sections
LDFLAGS := -specs=gba.specs $(ARCH)

all: $(TARGET).gba

$(TARGET).elf: source/main.c
	$(CC) $(CFLAGS) $< $(LDFLAGS) -o $@

$(TARGET).gba: $(TARGET).elf
	$(OBJCOPY) -O binary $< $@
	gbafix $@ -tBORE

clean:
	rm -f $(TARGET).elf $(TARGET).gba
.PHONY: all clean
