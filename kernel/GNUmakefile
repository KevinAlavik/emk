# EMK 1.0 Copyright (c) 2025 Piraterna
MAKEFLAGS += -rR
.SUFFIXES:

SRCDIR := src
OBJDIR := build
BINDIR := bin

TARGET := $(BINDIR)/emk.elf

KCONFIG ?= kconfig-mconf
CONFIG := .config

ifneq ("$(wildcard $(CONFIG))","")
    include $(CONFIG)
endif

CC := $(CONFIG_CC)
AS := $(CC)
NASM := nasm

ifeq ($(BUILD_MODE),)
    ifeq ($(CONFIG_BUILD_MODE_DEBUG),y)
        BUILD_MODE := debug
    else ifeq ($(CONFIG_BUILD_MODE_RELEASE),y)
        BUILD_MODE := release
    else
        BUILD_MODE := debug
    endif
endif

ifeq ($(BUILD_MODE),release)
    CFLAGS := -Os -pipe -Wall -Wextra -Werror -std=gnu11 -ffreestanding \
              -fno-stack-protector -fno-stack-check -fno-PIC \
              -ffunction-sections -fdata-sections -m64 -march=x86-64 \
              -mno-80387 -mno-mmx -mno-sse -mno-sse2 -mno-red-zone \
              -mcmodel=kernel -fno-unwind-tables -fno-asynchronous-unwind-tables \
              -s -Wno-unused-variable -DBUILD_MODE=\"release\"
    NASMFLAGS :=
    LDFLAGS := -nostdlib -static -z max-page-size=0x1000 -Wl,--gc-sections \
               -T linker.ld -Wl,-m,elf_x86_64 -Wl,--strip-all
else
    CFLAGS := -g -O2 -pipe -Wall -Wextra -Werror -std=gnu11 -ffreestanding \
              -fno-stack-protector -fno-stack-check -fno-PIC \
              -ffunction-sections -fdata-sections -m64 -march=x86-64 \
              -mno-80387 -mno-mmx -mno-sse -mno-sse2 -mno-red-zone \
              -mcmodel=kernel -Wno-unused-variable -DBUILD_MODE=\"dev\"
    NASMFLAGS := -F dwarf -g
    LDFLAGS := -nostdlib -static -z max-page-size=0x1000 -Wl,--gc-sections \
               -T linker.ld -Wl,-m,elf_x86_64
endif

CPPFLAGS := -I../external -I$(SRCDIR) -MMD -MP -DLIMINE_API_REVISION=3
CFLAGS +=  -std=c99

IMPLICIT_SRCS :=
ifneq ($(MAKECMDGOALS),menuconfig)
ifeq ($(CONFIG_KERNEL_HEAP_FF),y)
    IMPLICIT_SRCS += src/mm/heap/ff.c
else
    $(error Error: No heap algorithm was defined. Please run "make menuconfig" and select one.)
endif
endif

ifeq ($(CONFIG_ENABLE_FLANTERM),y)
    IMPLICIT_SRCS += ../external/flanterm/flanterm.c
    IMPLICIT_SRCS += ../external/flanterm/backends/fb.c
    CFLAGS += -I../external/flanterm/ -DFLANTERM_SUPPORT=1
endif

EXCLUDE_SRCS := \
    src/mm/heap/ff.c \
    ../external/flanterm/flanterm.c \
    ../external/flanterm/backends/fb.c
EXCLUDE_PATTERNS := $(foreach file,$(EXCLUDE_SRCS),! -path "$(file)")

SRCS := $(IMPLICIT_SRCS) \
        $(shell find $(SRCDIR) -type f \( -name '*.c' -o -name '*.S' -o -name '*.asm' \) $(EXCLUDE_PATTERNS))

define src_to_obj
$(OBJDIR)/$(patsubst ../%,%,$(1:.c=.o))
endef

define src_to_obj_s
$(OBJDIR)/$(patsubst ../%,%,$(1:.S=.o))
endef

define src_to_obj_asm
$(OBJDIR)/$(patsubst ../%,%,$(1:.asm=.o))
endef

OBJS := 
OBJS += $(foreach f,$(filter %.c,$(SRCS)),$(call src_to_obj,$(f)))
OBJS += $(foreach f,$(filter %.S,$(SRCS)),$(call src_to_obj_s,$(f)))
OBJS += $(foreach f,$(filter %.asm,$(SRCS)),$(call src_to_obj_asm,$(f)))

DEPS := $(OBJS:.o=.d)

.PHONY: all clean distclean menuconfig
all: $(TARGET)

$(OBJDIR) $(BINDIR):
	@mkdir -p $@

$(TARGET): $(OBJS) | $(BINDIR)
	@echo "  LD    $@"
	@$(CC) $(LDFLAGS) -o $@ $(OBJS)

$(OBJDIR)/%.o: ../%.c | $(OBJDIR)
	@mkdir -p $(dir $@)
	@echo "  CC    ../$*"
	@$(CC) $(CPPFLAGS) $(CFLAGS) -c ../$*.c -o $@

$(OBJDIR)/%.o: %.c | $(OBJDIR)
	@mkdir -p $(dir $@)
	@echo "  CC    $<"
	@$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(OBJDIR)/%.o: ../%.S | $(OBJDIR)
	@mkdir -p $(dir $@)
	@echo "  AS    ../$*"
	@$(AS) $(CPPFLAGS) $(CFLAGS) -c ../$*.S -o $@

$(OBJDIR)/%.o: %.S | $(OBJDIR)
	@mkdir -p $(dir $@)
	@echo "  AS    $<"
	@$(AS) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(OBJDIR)/%.o: ../%.asm | $(OBJDIR)
	@mkdir -p $(dir $@)
	@echo "  NASM  ../$*"
	@$(NASM) $(NASMFLAGS) ../$*.asm -o $@

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

menuconfig: clean
	@$(KCONFIG) Kconfig

-include $(DEPS)
