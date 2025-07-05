CC      ?= gcc
LD      ?= ld
SRC     = init.c
OBJ     = init.o
OUT     = init.sys

CFLAGS  = -ffreestanding -fno-builtin -fno-stack-protector -fno-pic -mno-red-zone -nostdlib
LDFLAGS = -nostdlib --no-dynamic-linker --strip-all -T linker.ld

all: $(OUT)

$(OBJ): $(SRC)
	@echo "  CC    $<"
	@$(CC) $(CFLAGS) -c $< -o $@

$(OUT): $(OBJ)
	@echo "  LD    $@"
	@$(LD) $(OBJ) -o $@ $(LDFLAGS)

clean:
	@rm -f $(OBJ) $(OUT)
