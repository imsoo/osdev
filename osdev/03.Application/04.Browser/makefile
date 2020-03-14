###################
# Build Set
###################
NASM64 = nasm -f elf64
GCC64 = x86_64-pc-linux-gcc -c -m64 -ffreestanding -mcmodel=large -fno-common
LD64 = x86_64-pc-linux-ld -melf_x86_64 -T ../elf_x86_64.x -nostdlib -e _START -Ttext 0x0000
LDFLAGS = -lOS -L../../UserLibrary
CFLAGS = -I../../UserLibrary/Source

OBJCOPY64 = x86_64-pc-linux-objcopy -j .text -j .data -j .rodata -j .bss

OBJECT_DIR = Temp
SOURCE_DIR = Source

###################
# Build Rule
###################

all: prepare Output.elf

prepare:
	mkdir -p $(OBJECT_DIR)


dep:
	@echo === Make Dependancy File ===
	make -C $(OBJECT_DIR) -f ../makefile InternalDependency
	@echo === Dependancy Search Complete ===

ExecuteInternalBuild: dep
	make -C $(OBJECT_DIR) -f ../makefile Main.elf

Output.elf: ExecuteInternalBuild
	$(OBJCOPY64) $(OBJECT_DIR)/Main.elf $@
		
clean:
	rm -f *.elf
	rm -f $(OBJECT_DIR)/*.*

C_SOURCE_FILE = $(wildcard ../$(SOURCE_DIR)/*.c)
ASM_SOURCE_FILE = $(wildcard ../$(SOURCE_DIR)/*.asm)
C_OBJECT_FILE = $(notdir $(patsubst %.c,%.o,$(C_SOURCE_FILE)))
ASM_OBJECT_FILE = $(notdir $(patsubst %.asm,%.o,$(ASM_SOURCE_FILE)))

%.o: ../$(SOURCE_DIR)/%.c
	$(GCC64) -c $< $(CFLAGS)

%.o: ../$(SOURCE_DIR)/%.asm
	$(NASM64) -o $@ $<

InternalDependency:
	$(GCC64) -MM $(C_SOURCE_FILE) > Dependency.dep $(CFLAGS)

Main.elf: $(C_OBJECT_FILE) $(ASM_OBJECT_FILE)
	$(LD64) -r -o $@ $^ $(LDFLAGS)

ifeq (Dependency.dep, $(wildcard Dependency.dep))
include Dependency.dep
endif
