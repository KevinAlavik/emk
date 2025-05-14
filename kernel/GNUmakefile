# EMK 1.0 Copyright (c) 2025 Piraterna
MAKEFLAGS += -rR
.SUFFIXES:

SRCDIR := src
OBJDIR := build
BINDIR := bin

TARGET := $(BINDIR)/emk.elf

CC := gcc
AS := gcc
NASM := nasm

# Build mode configuration
BUILD_MODE ?= dev
ifeq ($(BUILD_MODE),release)
    CFLAGS := -Os -pipe -Wall -Wextra -Werror -std=gnu11 -ffreestanding \
              -fno-stack-protector -fno-stack-check -fno-PIC \
              -ffunction-sections -fdata-sections -m64 -march=x86-64 \
              -mno-80387 -mno-mmx -mno-sse -mno-sse2 -mno-red-zone \
              -mcmodel=kernel -fno-unwind-tables -fno-asynchronous-unwind-tables \
              -s -Wno-unused-variable
    NASMFLAGS :=
    LDFLAGS := -nostdlib -static -z max-page-size=0x1000 -Wl,--gc-sections \
               -T linker.ld -Wl,-m,elf_x86_64 -Wl,--strip-all
else
    CFLAGS := -g -O2 -pipe -Wall -Wextra -Werror -std=gnu11 -ffreestanding \
              -fno-stack-protector -fno-stack-check -fno-PIC \
              -ffunction-sections -fdata-sections -m64 -march=x86-64 \
              -mno-80387 -mno-mmx -mno-sse -mno-sse2 -mno-red-zone \
              -mcmodel=kernel -Wno-unused-variable
    NASMFLAGS := -F dwarf -g
    LDFLAGS := -nostdlib -static -z max-page-size=0x1000 -Wl,--gc-sections \
               -T linker.ld -Wl,-m,elf_x86_64
endif

CPPFLAGS := -I../external -I$(SRCDIR) -MMD -MP -DLIMINE_API_REVISION=3

SRCS := $(shell find $(SRCDIR) -type f \( -name '*.c' -o -name '*.S' -o -name '*.asm' \))
OBJS := $(patsubst %.c,$(OBJDIR)/%.o,$(filter %.c,$(SRCS))) \
        $(patsubst %.S,$(OBJDIR)/%.o,$(filter %.S,$(SRCS))) \
        $(patsubst %.asm,$(OBJDIR)/%.o,$(filter %.asm,$(SRCS)))

DEPS := $(OBJS:.o=.d)

.PHONY: all clean distclean
all: $(TARGET)

$(OBJDIR) $(BINDIR):
	@mkdir -p $@

$(TARGET): $(OBJS) | $(BINDIR)
	@echo "  LD    $@"
	@$(CC) $(LDFLAGS) -o $@ $(OBJS)

$(OBJDIR)/%.o: %.c | $(OBJDIR)
	@mkdir -p $(dir $@)
	@echo "  CC    $<"
	@$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(OBJDIR)/%.o: %.S | $(OBJDIR)
	@mkdir -p $(dir $@)
	@echo "  AS    $<"
	@$(AS) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(OBJDIR)/%.o: %.asm | $(OBJDIR)
	@mkdir -p $(dir $@)
	@echo "  NASM  $<"
	@$(NASM) $(NASMFLAGS) $< -o $@

clean:
	@echo "  CLEAN"
	@rm -rf $(OBJDIR) $(TARGET)

distclean: clean
	@echo "  DISTCLEAN"
	@rm -rf $(BINDIR)

-include $(DEPS)