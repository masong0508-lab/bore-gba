# Needs devkitARM (arm-none-eabi-gcc + gbafix). Paths are set below so PATH does not matter.
DEVKITPRO ?= /opt/devkitpro
DEVKITARM ?= $(DEVKITPRO)/devkitARM
export PATH := $(DEVKITARM)/bin:$(DEVKITPRO)/tools/bin:$(PATH)

TARGET  := bore
CC      := arm-none-eabi-gcc
OBJCOPY := arm-none-eabi-objcopy
ARCH    := -mthumb -mthumb-interwork
CFLAGS  := $(ARCH) -O2 -Wall -fno-strict-aliasing -ffunction-sections -mcpu=arm7tdmi -mtune=arm7tdmi -fomit-frame-pointer -fno-unwind-tables -fno-asynchronous-unwind-tables
LDFLAGS := -specs=gba.specs $(ARCH)
STACK_EWRAM := 8192                       # the stack sits in the top of EWRAM (main.c: main() stub); statics must stay below it
EWRAM_STATIC_MAX := $(shell echo $$((262144-8192)))
IWRAM_MAX ?= 32512   # 32768 minus the top 256 B the BIOS uses; .bss + .data + .iwram must fit below (override: make IWRAM_MAX=n)

all: $(TARGET).gba

$(TARGET).elf: source/main.c source/logo.c $(wildcard source/*.h) $(wildcard source/sfx/*.adp) $(wildcard source/music/*.adp) $(wildcard source/music/*.bin)
	$(CC) $(CFLAGS) source/main.c source/logo.c $(LDFLAGS) -o $@
	@arm-none-eabi-size -A $@ | awk -v lim=$(EWRAM_STATIC_MAX) '$$1==".sbss"||$$1==".ewram"{e+=$$2} END{ if(e>lim){ printf("ERROR: EWRAM statics %d B > %d B: the top %d KB of EWRAM is the stack (see main.c)\n",e,lim,$(STACK_EWRAM)/1024); exit 1 } }'
	@arm-none-eabi-size -A $@ | awk -v lim=$(IWRAM_MAX) '$$1==".bss"||$$1==".data"||$$1==".iwram"{i+=$$2} END{ printf("IWRAM: %d B used of %d (%d B free)\n",i,lim,lim-i); if(i>lim){ printf("ERROR: IWRAM %d B > %d B: new code must not be IWRAM_*, new statics need EWRAM_BSS\n",i,lim); exit 1 } }'

$(TARGET).gba: $(TARGET).elf
	$(OBJCOPY) -O binary $< $@
	gbafix $@ -tBORE

clean:
	rm -f $(TARGET).elf $(TARGET).gba

# Memory report: EWRAM = .sbss (+ .ewram), IWRAM = .bss + .data + .iwram (the stack gets what is left of it), ROM = the .gba file.
# Prints a table (and appends it to the GitHub job summary when run in Actions).
size: $(TARGET).elf $(TARGET).gba
	@arm-none-eabi-size -A $(TARGET).elf | awk -v rom=$$(stat -c %s $(TARGET).gba) '\
	  $$1==".sbss"||$$1==".ewram"{e+=$$2} $$1==".bss"||$$1==".data"||$$1==".iwram"{i+=$$2} \
	  END{ printf("| memory | used | of | free |\n|---|---|---|---|\n"); \
	       printf("| EWRAM | %d B (%.1f%%) | 262144 | %d B |\n",e,e*100/262144,262144-e); \
	       printf("| IWRAM (before stack) | %d B (%.1f%%) | 32768 | %d B |\n",i,i*100/32768,32768-i); \
	       printf("| ROM | %d B (%.1f%%) | 33554432 | %d B |\n",rom,rom*100/33554432,33554432-rom) }' | tee mem-report.md
	@if [ -n "$$GITHUB_STEP_SUMMARY" ]; then { echo "### GBA memory"; cat mem-report.md; echo; echo '<details><summary>biggest EWRAM/IWRAM symbols</summary>'; echo; echo '```'; arm-none-eabi-nm -S --size-sort -r $(TARGET).elf | head -25; echo '```'; echo '</details>'; } >> "$$GITHUB_STEP_SUMMARY"; fi
.PHONY: all clean size
