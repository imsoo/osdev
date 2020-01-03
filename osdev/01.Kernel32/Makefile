###################
# Build Set
###################

NASM32 = nasm
GCC32 = x86_64-pc-linux-gcc -c -m32 -ffreestanding
LD32 = x86_64-pc-linux-ld -melf_i386 -T ../elf_i386.x -nostdlib -e Main -Ttext 0x10200
OBJCOPY32 = x86_64-pc-linux-objcopy -j .text -j .data -j .rodata -j .bss -S -O binary

OBJECT_DIR = Temp
SOURCE_DIR = Source

###################
# Build Rule
###################

all: prepare Kernel32.bin

prepare: 
	mkdir -p $(OBJECT_DIR)

$(OBJECT_DIR)/EntryPoint.bin: $(SOURCE_DIR)/EntryPoint.s
	$(NASM32) -o $@ $<

dep:
	@echo === Make Dependency File ===
	make -C $(OBJECT_DIR) -f ../makefile InternalDependency
	@echo === Dependency Search Complete ===

ExecuteInternalBuild: dep
	make -C $(OBJECT_DIR) -f ../makefile Kernel32.elf

$(OBJECT_DIR)/Kernel32.elf.bin: ExecuteInternalBuild
	$(OBJCOPY32) $(OBJECT_DIR)/Kernel32.elf $@

Kernel32.bin: $(OBJECT_DIR)/EntryPoint.bin $(OBJECT_DIR)/Kernel32.elf.bin
	cat $^ > $@

clean:
	rm -f *.bin
	rm -f $(OBJECT_DIR)/*.*

C_ENTRY_OBJ_FILE = Main.o
C_SOURCE_FILE = $(wildcard ../$(SOURCE_DIR)/*.c)
ASM_SOURCE_FILE = $(wildcard ../$(SOURCE_DIR)/*.asm)
C_OBJECT_FILE = $(subst Main.o, ,$(notdir $(patsubst %.c,%.o,$(C_SOURCE_FILE))))
ASM_OBJECT_FILE = $(notdir $(patsubst %.asm,%.o,$(ASM_SOURCE_FILE)))

# .c to .o
%.o: ../$(SOURCE_DIR)/%.c
	$(GCC32) -c $<

# .asm to .o
%.o: ../$(SOURCE_DIR)/%.asm
	$(NASM32) -f elf32 -o $@ $<

InternalDependency:
	$(GCC32) -MM $(C_SOURCE_FILE) > Dependency.dep

Kernel32.elf: $(C_ENTRY_OBJ_FILE) $(C_OBJECT_FILE) $(ASM_OBJECT_FILE)
	$(LD32) -o $@ $^

ifeq (Dependency.dep, $(wildcard Dependency.dep))
include Dependency.dep
endif
