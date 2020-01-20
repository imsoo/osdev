###################
# Build Set
###################

NASM64 = nasm -f elf64
GCC64 = x86_64-pc-linux-gcc -c -m64 -ffreestanding -mcmodel=large -fno-common
AR64 = x86_64-pc-linux-ar rcs

OBJECT_DIR = Temp
SOURCE_DIR = Source

###################
# Build Rule
###################
all: prepare libOS

prepare:
	mkdir -p $(OBJECT_DIR)

dep:
	@echo === Make Dependancy File ===
	make -C $(OBJECT_DIR) -f ../makefile InternalDependency
	@echo === Dependancy Search Complete ===

libOS: dep
	make -C $(OBJECT_DIR) -f ../makefile libOS.a
	cp $(OBJECT_DIR)/libOS.a ./
		
clean:
	rm -f *.a
	rm -f $(OBJECT_DIR)/*.*

C_SOURCE_FILE = $(wildcard ../$(SOURCE_DIR)/*.c)
ASM_SOURCE_FILE = $(wildcard ../$(SOURCE_DIR)/*.asm)
C_OBJECT_FILE = $(notdir $(patsubst %.c,%.o,$(C_SOURCE_FILE)))
ASM_OBJECT_FILE = $(notdir $(patsubst %.asm,%.o,$(ASM_SOURCE_FILE)))

%.o: ../$(SOURCE_DIR)/%.c
	$(GCC64) -c $<

%.o: ../$(SOURCE_DIR)/%.asm
	$(NASM64) -o $@ $<

InternalDependency:
	$(GCC64) -MM $(C_SOURCE_FILE) > Dependency.dep

libOS.a: $(ASM_OBJECT_FILE) $(C_OBJECT_FILE)
	$(AR64) $@ $^

ifeq (Dependency.dep, $(wildcard Dependency.dep))
include Dependency.dep
endif
