###################
# Build Set
###################

NASM64 = nasm -f elf64
GCC64 = x86_64-pc-linux-gcc -c -m64 -ffreestanding
LD64 = x86_64-pc-linux-ld -melf_x86_64 -T ../elf_x86_64.x -nostdlib -e Main -Ttext 0x200000
OBJCOPY64 = x86_64-pc-linux-objcopy -j .text -j .data -j .rodata -j .bss -S -O binary

OBJECT_DIR = Temp
SOURCE_DIR = Source

###################
# Build Rule
###################

all: prepare Kernel64.bin

prepare: 
	mkdir -p $(OBJECT_DIR)

dep:
	@echo === Make Dependency File ===
	make -C $(OBJECT_DIR) -f ../makefile InternalDependency
	@echo === Dependency Search Complete ===

ExecuteInternalBuild: dep
	make -C $(OBJECT_DIR) -f ../makefile Kernel64.elf

Kernel64.bin: ExecuteInternalBuild
	$(OBJCOPY64) $(OBJECT_DIR)/Kernel64.elf $@

clean:
	rm -f *.bin
	rm -f $(OBJECT_DIR)/*.*

ENTRY_POINT_SOURCE_FILE = ../$(SOURCE_DIR)/EntryPoint.s
ENTRY_POINT_OBJECT_FILE = EntryPoint.o

C_SOURCE_FILE = $(wildcard ../$(SOURCE_DIR)/*.c)
ASM_SOURCE_FILE = $(wildcard ../$(SOURCE_DIR)/*.asm)
C_OBJECT_FILE = $(notdir $(patsubst %.c,%.o,$(C_SOURCE_FILE)))
ASM_OBJECT_FILE = $(notdir $(patsubst %.asm,%.o,$(ASM_SOURCE_FILE)))

$(ENTRY_POINT_OBJECT_FILE): $(ENTRY_POINT_SOURCE_FILE)
	$(NASM64) -o $@ $<

# .c to .o
%.o: ../$(SOURCE_DIR)/%.c
	$(GCC64) -c $<

# .asm to .o
%.o: ../$(SOURCE_DIR)/%.asm
	$(NASM64) -o $@ $<

InternalDependency:
	$(GCC64) -MM $(C_SOURCE_FILE) > Dependency.dep

Kernel64.elf: $(ENTRY_POINT_OBJECT_FILE) $(C_OBJECT_FILE) $(ASM_OBJECT_FILE)
	$(LD64) -o $@ $^

ifeq (Dependency.dep, $(wildcard Dependency.dep))
include Dependency.dep
endif
