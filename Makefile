SRC_DIR ?= ./src
OBJ_DIR ?= ./build
INCLUDE_DIR ?= ./include
SOURCES ?= $(shell find $(SRC_DIR) -name '*.c' -or -name '*.S')
OBJECTS ?= $(addprefix $(OBJ_DIR)/, $(notdir $(SOURCES:.c=.o) $(SOURCES:.S=.o)))
LINKER ?= $(SRC_DIR)/dtekv-script.lds

TOOLCHAIN ?= riscv32-unknown-elf-
CFLAGS ?= -Wall -nostdlib -O3 -mabi=ilp32 -march=rv32imzicsr -fno-builtin -I$(INCLUDE_DIR)


build: clean main.bin

main.elf: $(OBJECTS)
    $(TOOLCHAIN)ld -o $@ -T $(LINKER) $(filter-out $(OBJ_DIR)/boot.o, $^) softfloat.a

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
    @mkdir -p $(OBJ_DIR)
    $(TOOLCHAIN)gcc -c $(CFLAGS) $< -o $@

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.S
    @mkdir -p $(OBJ_DIR)
    $(TOOLCHAIN)gcc -c $(CFLAGS) $< -o $@

main.bin: main.elf
	$(TOOLCHAIN)objcopy --output-target binary $< $@
	$(TOOLCHAIN)objdump -D $< > $<.txt

clean:
	rm -f *.o *.elf *.bin *.txt

TOOL_DIR ?= ./tools
run: main.bin
	make -C $(TOOL_DIR) "FILE_TO_RUN=$(CURDIR)/$<"
